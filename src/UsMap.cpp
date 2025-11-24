#include "UsMap.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QRegularExpression>
#include <QXmlStreamReader>
#include <QtMath>
#include <cstdint>
#include <qglobal.h>
#include <qstringliteral.h>
#include <qstringview.h>

UsMap::UsMap(QString svgFilePath, int cols, int rows)
    : m_svgFilePath(std::move(svgFilePath)), m_outputProducts({{}, {}, cols, rows})
{
    m_outputProducts.activeStates.resize(
        static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows), 0);
    m_outputProducts.stateIds.resize(
        static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows), kNoState);
    m_statePixelCount.clear();
}

bool UsMap::buildProducts(QString* errorMessage)
{
    if (!loadSvgPatched(errorMessage))
    {
        if (errorMessage)
            *errorMessage = "QSvgRenderer: failed to load " + m_svgFilePath;
        return false;
    }

    if (!scanStatesFromSvg())
    {
        qWarning() << "[UsMap] No states parsed from SVG.";
    }

    const int cols = m_outputProducts.cols, rows = m_outputProducts.rows;
    m_statePixelCount.assign(m_states.size(), 0);
    std::fill(m_outputProducts.stateIds.begin(), m_outputProducts.stateIds.end(), kNoState);

    m_maskImage = QImage(cols, rows, QImage::Format_Grayscale8);
    m_maskImage.fill(Qt::black);
    {
        QPainter pm(&m_maskImage);
        pm.setRenderHint(QPainter::Antialiasing, false);
        pm.setCompositionMode(QPainter::CompositionMode_Source);
        m_svgRenderer.render(&pm, QRectF(0, 0, cols, rows));
    }
    for (int y = 0; y < rows; ++y)
    {
        const uchar* line = m_maskImage.constScanLine(y);
        for (int x = 0; x < cols; ++x)
        {
            m_outputProducts.activeStates[static_cast<std::size_t>(y * cols + x)] =
                (line[x] > 0) ? 1 : 0;
        }
    }
    if (m_debugEnabled)
    {
        debugSave("01_mask.png", m_maskImage);
    }

    QImage colorImage(cols, rows, QImage::Format_ARGB32_Premultiplied);
    colorImage.fill(Qt::transparent);
    {
        QPainter pc(&colorImage);
        pc.setRenderHint(QPainter::Antialiasing, false);
        pc.setCompositionMode(QPainter::CompositionMode_Source);
        m_svgRenderer.render(&pc, QRectF(0, 0, cols, rows));
    }
    if (m_debugEnabled)
    {
        debugSave("02_color_full.png", colorImage);
    }

    m_colorToState.clear();

    auto quant = [](QRgb c) { return qRgb(qRed(c) & 0xF8, qGreen(c) & 0xF8, qBlue(c) & 0xF8); };

    QImage oneState(cols, rows, QImage::Format_ARGB32_Premultiplied);

    for (int sid = 0; sid < static_cast<int>(m_states.size()); ++sid)
    {
        const QString elemId = m_states[static_cast<std::size_t>(sid)].id;
        if (!m_svgRenderer.elementExists(elemId))
        {
            qWarning() << "[UsMap] Missing element:" << elemId;
            continue;
        }

        oneState.fill(Qt::transparent);
        {
            QPainter p(&oneState);
            p.setRenderHint(QPainter::Antialiasing, false);
            p.setCompositionMode(QPainter::CompositionMode_Source);
            m_svgRenderer.render(&p, elemId, QRectF(0, 0, cols, rows));
        }

        if (m_debugEnabled)
        {
            debugSave(QString("state_%1.png").arg(elemId), oneState);
        }

        QHash<QRgb, int> hist;
        int              totalAlphaPixels = 0;
        for (int y = 0; y < rows; ++y)
        {
            const QRgb* line = reinterpret_cast<const QRgb*>(oneState.constScanLine(y));
            for (int x = 0; x < cols; ++x)
            {
                QRgb c = line[x];
                if (qAlpha(c) > 160)
                {
                    ++totalAlphaPixels;
                    hist[quant(c)] += 1;
                }
            }
        }

        if (totalAlphaPixels == 0)
        {
            qWarning() << "[UsMap] Empty render for" << elemId << "(transparent?)";
            continue;
        }

        QRgb best    = 0;
        int  bestCnt = -1;
        for (auto it = hist.constBegin(); it != hist.constEnd(); ++it)
        {
            if (it.value() > bestCnt)
            {
                bestCnt = it.value();
                best    = it.key();
            }
        }

        if (m_colorToState.contains(best) &&
            m_colorToState.value(best) != static_cast<uint8_t>(sid))
        {
            const uint8_t otherSid = m_colorToState.value(best);
            const QString otherId =
                (otherSid < m_states.size()) ? m_states[otherSid].id : QStringLiteral("?");
            qWarning() << "[UsMap] COLOR CONFLICT"
                       << QString("#%1").arg((qRed(best) << 16) | (qGreen(best) << 8) | qBlue(best),
                                             6, 16, QChar('0'))
                       << "between" << otherId << "and" << elemId << "(ensure unique fills in SVG)";
        }
        else
        {
            m_colorToState.insert(best, static_cast<uint8_t>(sid));
            if (m_debugEnabled)
            {
                qDebug() << "[UsMap] color map (by render):" << elemId
                         << QString("#%1").arg((qRed(best) << 16) | (qGreen(best) << 8) |
                                                   qBlue(best),
                                               6, 16, QChar('0'))
                         << "pixels:" << totalAlphaPixels;
            }
        }
    }

    std::fill(m_outputProducts.stateIds.begin(), m_outputProducts.stateIds.end(), kNoState);
    std::fill(m_statePixelCount.begin(), m_statePixelCount.end(), 0);

    for (int y = 0; y < rows; ++y)
    {
        const QRgb* cl = reinterpret_cast<const QRgb*>(colorImage.constScanLine(y));
        for (int x = 0; x < cols; ++x)
        {
            const int i = y * cols + x;
            if (!m_outputProducts.activeStates[static_cast<std::size_t>(i)])
            {
                continue;
            }
            if (qAlpha(cl[x]) == 0)
            {
                continue;
            }

            const QRgb key = quant(cl[x]);
            auto       it  = m_colorToState.constFind(key);
            if (it != m_colorToState.constEnd())
            {
                const uint8_t sid                                      = it.value();
                m_outputProducts.stateIds[static_cast<std::size_t>(i)] = sid;
                m_statePixelCount[sid] += 1;
            }
        }
    }

    m_productsBuilt = true;
    return true;
}

void UsMap::setError(QString* errorMessage, const QString& message) const
{
    if (errorMessage)
    {
        *errorMessage = message;
    }
}

bool UsMap::readSvgFile(QString* errorMessage)
{
    QFile mapFile(m_svgFilePath);

    if (!mapFile.open(QIODevice::ReadOnly))
    {
        setError(errorMessage, "Cannot open " + m_svgFilePath);
        return false;
    }

    m_svgRaw = mapFile.readAll();
    return true;
}

void UsMap::patchSvgContent()
{
    constexpr const char* strokes[] = {
        "stroke:#000000",
        "stroke:#000",
        "stroke:black",
    };

    for (const auto& stroke : strokes)
    {
        m_svgRaw.replace(stroke, "stroke:none");
    }
}

bool UsMap::loadSvgIntoRenderer(QString* errorMessage)
{
    if (!m_svgRenderer.load(m_svgRaw))
    {
        setError(errorMessage, "QSvgRenderer: failed to load patched svg");
        return false;
    }

    return true;
}

bool UsMap::loadSvgPatched(QString* errorMessage)
{
    if (!readSvgFile(errorMessage))
    {
        return false;
    }

    patchSvgContent();

    if (!loadSvgIntoRenderer(errorMessage))
    {
        return false;
    }

    return true;
}

bool UsMap::isValidHexColor(const QString& hex)
{
    const QRegularExpression regex("^#[0-9A-Fa-f]{6}$");
    return regex.match(hex).hasMatch();
}

std::optional<QRgb> UsMap::parseHexColor(const QString& hex)
{
    if (!isValidHexColor(hex))
    {
        return std::nullopt;
    }
    bool ok  = false;
    uint val = hex.mid(1).toUInt(&ok, 16);
    if (!ok)
    {
        return std::nullopt;
    }

    int r = (val >> 16) & 0xFF;
    int g = (val >> 8) & 0xFF;
    int b = (val) & 0xFF;
    return qRgb(r, g, b);
}

void UsMap::resetStates()
{
    m_states.clear();
    m_stateIdToIndex.clear();
}

bool UsMap::isStateElement(const QString& tag) const
{
    static constexpr const char* tags[] = {"path", "polygon", "rect", "g"};

    for (const char* t : tags)
    {
        if (tag.compare(QLatin1StringView(t), Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

QString UsMap::extractFill(const QXmlStreamAttributes& attrs) const
{
    if (const auto fill = attrs.value("fill"); !fill.isEmpty())
    {
        return fill.toString().trimmed();
    }

    if (const auto style = attrs.value("style"); !style.isEmpty())
    {
        for (const auto& kv : style.toString().split(';', Qt::SkipEmptyParts))
        {
            const int pos = kv.indexOf(':');
            if (pos > 0 && kv.left(pos).trimmed().compare("fill", Qt::CaseInsensitive) == 0)
                return kv.mid(pos + 1).trimmed();
        }
    }
    return {};
}

void UsMap::processStateElement(const QXmlStreamAttributes& attrs)
{
    State state;
    state.id = attrs.value("id").toString().trimmed();
    if (state.id.isEmpty())
        return;

    state.name = attrs.value("data-name").toString().trimmed();

    if (auto rgb = parseHexColor(extractFill(attrs)))
        state.fill = *rgb;

    insertOrUpdateState(std::move(state));
}

void UsMap::insertOrUpdateState(State&& st)
{
    if (m_stateIdToIndex.contains(st.id))
    {
        int idx = m_stateIdToIndex.value(st.id);
        if (m_states[idx].name.isEmpty() && !st.name.isEmpty())
            m_states[idx].name = st.name;
        return;
    }

    int idx = static_cast<int>(m_states.size());
    m_stateIdToIndex.insert(st.id, idx);
    m_states.push_back(std::move(st));
}

void UsMap::reportXmlError(const QXmlStreamReader& xml) const
{
    if (xml.hasError())
        qWarning() << "[UsMap] XML parse error:" << xml.errorString();
}

bool UsMap::scanStatesFromSvg()
{
    resetStates();

    QXmlStreamReader xml(m_svgRaw);

    while (!xml.atEnd())
    {
        xml.readNext();

        if (!xml.isStartElement())
            continue;

        const QString tagName = xml.name().toString();
        if (!isStateElement(tagName))
            continue;

        processStateElement(xml.attributes());
    }

    reportXmlError(xml);
    return !m_states.empty();
}

void UsMap::drawBackground(QPainter& painter, const QRect& rect) const
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (m_debugColorize && !m_debugColorImage.isNull())
    {
        painter.drawImage(rect, m_debugColorImage);
    }
    else
    {
        m_svgRenderer.render(&painter, rect);
    }

    painter.restore();
}

uint8_t UsMap::stateAtViewPos(QPointF position, QSize viewSize) const
{
    if (!m_productsBuilt || viewSize.width() <= 0 || viewSize.height() <= 0)
    {
        return kNoState;
    }
    const auto sx = static_cast<qreal>(m_outputProducts.cols) / viewSize.width();
    const auto sy = static_cast<qreal>(m_outputProducts.rows) / viewSize.height();
    const int  x  = static_cast<int>(position.x() * sx);
    const int  y  = static_cast<int>(position.y() * sy);

    if (x < 0 || y < 0 || x >= m_outputProducts.cols || y >= m_outputProducts.rows)
    {
        return kNoState;
    }
    return m_outputProducts.stateIds[static_cast<std::size_t>(y * m_outputProducts.cols + x)];
}

UsMap::Products& UsMap::getProducts() const
{
    return const_cast<UsMap::Products&>(m_outputProducts);
}

void UsMap::setDebug(bool on, QString dumpDir)
{
    m_debugEnabled = on;
    m_debugDir     = dumpDir.isEmpty() ? QDir::tempPath() + "/usmap_debug" : std::move(dumpDir);
    if (m_debugEnabled)
    {
        QDir().mkpath(m_debugDir);
        qDebug() << "[UsMap] Debug enabled. Dump dir:" << m_debugDir;
    }
}

void UsMap::setDebugColorizeStates(bool on)
{
    m_debugColorize = on;
}

void UsMap::debugSave(const QString& name, const QImage& img) const
{
    if (!m_debugEnabled)
    {
        return;
    }
    const QString path = m_debugDir + "/" + name;
    img.save(path);
    qDebug() << "[UsMap] saved" << path;
}

QString UsMap::getStateName(uint8_t stateId) const
{
    if (stateId == kNoState || stateId >= m_states.size())
    {
        return {};
    }
    const auto& st = m_states[stateId];
    return st.name.isEmpty() ? st.id : st.name;
}

QString UsMap::getStateId(uint8_t stateId) const
{
    if (stateId == kNoState || stateId >= m_states.size())
    {
        return {};
    }
    return m_states[stateId].id;
}

std::optional<int> UsMap::indexOfAbbrev(QStringView abbrev) const
{
    const QString key = abbrev.toString().trimmed().toUpper();
    const auto    it  = m_stateIdToIndex.constFind(key);
    if (it == m_stateIdToIndex.constEnd())
        return std::nullopt;
    return it.value();
}

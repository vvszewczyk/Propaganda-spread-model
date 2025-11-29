#include "UsMap.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QRegularExpression>
#include <QXmlStreamReader>
#include <QtMath>
#include <cstddef>
#include <cstdint>
#include <qglobal.h>
#include <qrgb.h>
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

bool UsMap::loadAndParseSvg(QString* errorMessage)
{
    if (!loadSvgPatched(errorMessage))
    {
        setError(errorMessage, "QSvgRenderer: failed to load " + m_svgFilePath);
        return false;
    }

    if (!scanStatesFromSvg())
    {
        qWarning() << "[UsMap] No states parsed from SVG.";
    }

    if (!loadSvgOutlines(errorMessage))
    {
        return false;
    }

    return true;
}

void UsMap::drawStateOutlines(QPainter& painter, const QRect& rect) const
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (!m_svgOutlineRenderer.isValid())
    {
        painter.restore();
        return;
    }

    m_svgOutlineRenderer.render(&painter, rect);

    painter.restore();
}

void UsMap::buildMaskAndActiveStates(int cols, int rows)
{
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
        debugSave(QStringLiteral("01_mask.png"), m_maskImage);
    }
}

QImage UsMap::renderColorImage(int cols, int rows) const
{
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
        debugSave(QStringLiteral("02_color_full.png"), colorImage);
    }

    return colorImage;
}

QRgb UsMap::quantColor(QRgb c) const
{
    return qRgb(qRed(c) & 0xF8, qGreen(c) & 0xF8, qBlue(c) & 0xF8);
}

QString UsMap::rgbToHex(QRgb c) const
{
    const int rgb = (qRed(c) << 16) | (qGreen(c) << 8) | qBlue(c);
    return QStringLiteral("#%1").arg(rgb, 6, 16, QChar('0'));
}

void UsMap::renderSingleState(const QString& elemId, QImage& target, int cols, int rows) const
{
    target.fill(Qt::transparent);
    {
        QPainter p(&target);
        p.setRenderHint(QPainter::Antialiasing, false);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        m_svgRenderer.render(&p, elemId, QRectF(0, 0, cols, rows));
    }
}

std::optional<QRgb> UsMap::findDominantQuantizedColor(const QImage& img,
                                                      int&          totalAlphaPixels) const
{
    QHash<QRgb, int> hist;
    totalAlphaPixels = 0;
    const int cols   = img.width();
    const int rows   = img.height();

    for (int y = 0; y < rows; ++y)
    {
        const QRgb* line = reinterpret_cast<const QRgb*>(img.constScanLine(y));
        for (int x = 0; x < cols; ++x)
        {
            QRgb c = line[x];
            if (qAlpha(c) > 160)
            {
                ++totalAlphaPixels;
                hist[quantColor(c)] += 1;
            }
        }
    }

    if (totalAlphaPixels == 0)
    {
        return std::nullopt;
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

    return best;
}

void UsMap::handleStateColorMapping(QRgb           best,
                                    uint8_t        stateId,
                                    const QString& elemId,
                                    int            totalAlphaPixels)
{
    auto it = m_colorToState.constFind(best);
    if (it != m_colorToState.constEnd() && it.value() != static_cast<uint8_t>(stateId))
    {
        const uint8_t otherSid = it.value();
        const QString otherId =
            (otherSid < m_states.size()) ? m_states[otherSid].id : QStringLiteral("?");
        qWarning() << "[UsMap] COLOR CONFLICT" << rgbToHex(best) << "between" << otherId << "and"
                   << elemId << "(ensure unique fills in SVG)";
        return;
    }

    m_colorToState.insert(best, static_cast<uint8_t>(stateId));

    if (m_debugEnabled)
    {
        qDebug() << "[UsMap] color map (by analysis):" << elemId << rgbToHex(best)
                 << "pixels:" << totalAlphaPixels;
    }
}

void UsMap::buildColorToStateMap(int cols, int rows)
{
    m_colorToState.clear();

    QImage oneState(cols, rows, QImage::Format_ARGB32_Premultiplied);

    for (int sid = 0; sid < static_cast<int>(m_states.size()); ++sid)
    {
        const auto&   state  = m_states[static_cast<std::size_t>(sid)];
        const QString elemId = state.id;

        if (!m_svgRenderer.elementExists(elemId))
        {
            qWarning() << "[UsMap] Missing element:" << elemId;
            continue;
        }

        renderSingleState(elemId, oneState, cols, rows);

        if (m_debugEnabled)
        {
            debugSave(QString("state_%1.png").arg(elemId), oneState);
        }

        int  totalAlphaPixels = 0;
        auto dominantBest     = findDominantQuantizedColor(oneState, totalAlphaPixels);
        if (!dominantBest)
        {
            qWarning() << "[UsMap] Empty render for" << elemId << "(transparent?)";
            continue;
        }

        handleStateColorMapping(*dominantBest, static_cast<uint8_t>(sid), elemId, totalAlphaPixels);
    }
}

void UsMap::initProductsBuffers()
{
    m_statePixelCount.assign(m_states.size(), 0);
    std::fill(m_outputProducts.stateIds.begin(), m_outputProducts.stateIds.end(), kNoState);
}

void UsMap::assignStatesFromColorImage(const QImage& colorImage, int cols, int rows)
{
    initProductsBuffers();

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

            const QRgb key = quantColor(cl[x]);
            auto       it  = m_colorToState.constFind(key);
            if (it == m_colorToState.constEnd())
            {
                continue;
            }
            const uint8_t     sid            = it.value();
            const std::size_t index          = static_cast<std::size_t>(i);
            m_outputProducts.stateIds[index] = sid;
            m_statePixelCount[sid] += 1;
        }
    }
}

bool UsMap::buildStateProducts(QString* errorMessage)
{
    if (!loadAndParseSvg(errorMessage))
    {
        return false;
    }

    const int cols = m_outputProducts.cols;
    const int rows = m_outputProducts.rows;

    buildMaskAndActiveStates(cols, rows);
    const QImage colorImage = renderColorImage(cols, rows);
    buildColorToStateMap(cols, rows);
    assignStatesFromColorImage(colorImage, cols, rows);

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

    m_svgRawOriginal = mapFile.readAll();
    m_svgRaw         = m_svgRawOriginal;
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

static QString makeOutlineSvg(const QByteArray& original)
{
    QString xml = QString::fromUtf8(original);
    {
        QRegularExpression re(R"(fill\s*:[^;"]*)", QRegularExpression::CaseInsensitiveOption);
        xml.replace(re, "fill:none");
    }
    {
        QRegularExpression re(R"(fill\s*=\s*"[^"]*")", QRegularExpression::CaseInsensitiveOption);
        xml.replace(re, R"(fill="none")");
    }

    return xml;
}

bool UsMap::loadSvgOutlines(QString* errorMessage)
{
    if (m_svgRawOriginal.isEmpty())
    {
        if (!readSvgFile(errorMessage))
            return false;
    }

    const QString    outlineSvg = makeOutlineSvg(m_svgRawOriginal);
    const QByteArray bytes      = outlineSvg.toUtf8();

    if (!m_svgOutlineRenderer.load(bytes))
    {
        setError(errorMessage, "QSvgRenderer: failed to load outlines for " + m_svgFilePath);
        return false;
    }

    return true;
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
            const int pos = static_cast<int>(kv.indexOf(':'));
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
        std::size_t idx = static_cast<std::size_t>(m_stateIdToIndex.value(st.id));
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
        {
            continue;
        }

        processStateElement(xml.attributes());
    }

    reportXmlError(xml);
    return !m_states.empty();
}

bool UsMap::isViewValid(QSize viewSize) const
{
    return m_productsBuilt && viewSize.width() > 0 && viewSize.height() > 0;
}

QPoint UsMap::mapViewToProduct(QPointF position, QSize viewSize) const
{
    const auto sx = static_cast<qreal>(m_outputProducts.cols) / viewSize.width();
    const auto sy = static_cast<qreal>(m_outputProducts.rows) / viewSize.height();
    return {static_cast<int>(position.x() * sx), static_cast<int>(position.y() * sy)};
}

bool UsMap::isInProductBounds(QPoint point) const
{
    return point.x() >= 0 && point.y() >= 0 && point.x() < m_outputProducts.cols &&
           point.y() < m_outputProducts.rows;
}

uint8_t UsMap::stateAtViewPos(QPointF position, QSize viewSize) const
{
    if (!isViewValid(viewSize))
    {
        return kNoState;
    }

    const auto productPoints = mapViewToProduct(position, viewSize);

    if (!isInProductBounds(productPoints))
    {
        return kNoState;
    }

    const auto index =
        static_cast<std::size_t>(productPoints.y() * m_outputProducts.cols + productPoints.x());

    return m_outputProducts.stateIds[index];
}

const UsMap::Products& UsMap::getProducts() const
{
    return m_outputProducts;
}

void UsMap::setDebug(bool enabled, QString dumpDir)
{
    m_debugEnabled = enabled;

    m_debugDir = dumpDir.isEmpty() ? QDir::tempPath() + "/usmap_debug" : std::move(dumpDir);

    if (!m_debugEnabled)
    {
        return;
    }

    QDir().mkpath(m_debugDir);
    qDebug() << "[UsMap] Debug enabled. Dump dir:" << m_debugDir;
}

void UsMap::setDebugColorizeStates(bool on)
{
    m_debugColorize = on;
}

void UsMap::debugSave(const QString& name, const QImage& img) const
{
    if (!m_debugEnabled || name.isEmpty())
    {
        return;
    }
    const QString path = m_debugDir + "/" + name;

    if (img.save(path))
    {
        qDebug() << "[UsMap] saved" << path;
    }
    else
    {
        qWarning() << "[UsMap] failed to save" << path;
    }
}

bool UsMap::isValidStateId(uint8_t stateId) const
{
    return stateId != kNoState && stateId < m_states.size();
}

QString UsMap::getStateName(uint8_t stateId) const
{
    if (!isValidStateId(stateId))
    {
        return {};
    }
    const auto& st = m_states[stateId];
    return st.name.isEmpty() ? st.id : st.name;
}

QString UsMap::getStateId(uint8_t stateId) const
{
    if (!isValidStateId(stateId))
    {
        return {};
    }
    return m_states[stateId].id;
}

std::optional<int> UsMap::indexOfAbbrev(QStringView abbrev) const
{
    const QString key = abbrev.toString().trimmed().toUpper();

    const auto it = m_stateIdToIndex.constFind(key);
    if (it == m_stateIdToIndex.constEnd())
    {
        return std::nullopt;
    }
    return it.value();
}

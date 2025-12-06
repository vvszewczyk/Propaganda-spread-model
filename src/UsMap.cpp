#include "UsMap.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QRegularExpression>
#include <QXmlStreamReader>
#include <QtMath>
#include <cstddef>
#include <cstdint>
#include <qbytearrayview.h>
#include <qdebug.h>
#include <qglobal.h>
#include <qrgb.h>
#include <qstringliteral.h>
#include <qstringview.h>
#include <span>

UsMap::UsMap(QString svgFilePath, int cols, int rows)
    : m_svgFilePath(std::move(svgFilePath)), m_outputProducts({{}, {}, cols, rows})
{
    m_outputProducts.activeStates.resize(
        static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows), 0);
    m_outputProducts.stateIds.resize(
        static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows), kNoState);
    m_statePixelCount.clear();

    if (m_debugEnabled && m_debugDir.isEmpty())
    {
        setDebug(m_debugEnabled);
    }
}

bool UsMap::loadAndParseSvg(QString* errorMessage)
{
    if (!loadSvgPatched(errorMessage))
    {
        setError(errorMessage, "[UsMap] QSvgRenderer: failed to load " + m_svgFilePath);
        return false;
    }

    if (!scanStatesFromSvg())
    {
        qWarning() << "[UsMap] No states parsed from SVG.";
    }

    if (!loadSvgOutlineWithoutFill(errorMessage))
    {
        setError(errorMessage,
                 "[UsMap] QSvgRenderer: failed to load outlines for " + m_svgFilePath);
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
        QPainter painter(&m_maskImage);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        m_svgRenderer.render(&painter, QRectF(0, 0, cols, rows));
    }
    for (int y = 0; y < rows; ++y)
    {
        const uchar*           linePtr = m_maskImage.constScanLine(y);
        std::span<const uchar> line{linePtr, static_cast<std::size_t>(cols)};
        for (int x = 0; x < cols; ++x)
        {
            m_outputProducts.activeStates[static_cast<std::size_t>(y * cols + x)] =
                (line[static_cast<std::size_t>(x)] > 0) ? 1 : 0;
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
        QPainter painter(&colorImage);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        m_svgRenderer.render(&painter, QRectF(0, 0, cols, rows));
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
        QPainter painter(&target);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        m_svgRenderer.render(&painter, elemId, QRectF(0, 0, cols, rows));
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

    for (int stateId = 0; stateId < static_cast<int>(m_states.size()); ++stateId)
    {
        const auto&   state  = m_states[static_cast<std::size_t>(stateId)];
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

        handleStateColorMapping(*dominantBest, static_cast<uint8_t>(stateId), elemId,
                                totalAlphaPixels);
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
        setError(errorMessage, "[UsMap] Cannot open " + m_svgFilePath);
        return false;
    }

    m_svgRawOriginal = mapFile.readAll();
    m_svgRaw         = m_svgRawOriginal;
    return true;
}

static QByteArray removeStatesFills(const QByteArray& original)
{
    QString xml = QString::fromUtf8(original);

    QRegularExpression regex(R"(fill\s*:[^;"]*)");

    xml.replace(regex, "fill:none");

    return xml.toUtf8();
}

bool UsMap::loadSvgOutlineWithoutFill(QString* errorMessage)
{
    if (m_svgRawOriginal.isEmpty())
    {
        if (!readSvgFile(errorMessage))
        {
            return false;
        }
    }

    const QByteArray statesWithoutFill = removeStatesFills(m_svgRawOriginal);

    if (!m_svgOutlineRenderer.load(statesWithoutFill))
    {
        setError(errorMessage,
                 "[UsMap] QSvgRenderer: failed to load outlines for " + m_svgFilePath);
        return false;
    }

    return true;
}

void UsMap::removeStatesOutlines()
{
    m_svgRaw.replace("stroke:#000", "stroke:none");
}

bool UsMap::loadSvgIntoRenderer(QString* errorMessage)
{
    if (!m_svgRenderer.load(m_svgRaw))
    {
        setError(errorMessage, "[UsMap] QSvgRenderer: failed to load patched svg");
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

    removeStatesOutlines();

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
    if (tag == QLatin1StringView("path"))
    {
        return true;
    }
    return false;
}

QString UsMap::extractFill(const QXmlStreamAttributes& attributes) const
{
    const auto style = attributes.value("style");
    if (style.isEmpty())
    {
        return {};
    }

    // "stroke-width:0.97063118;fill:#HexColor"
    for (const auto& stylePart : style.toString().split(';', Qt::SkipEmptyParts))
    {
        const int position = static_cast<int>(stylePart.indexOf(':'));
        if (position > 0 && stylePart.left(position) == QLatin1StringView("fill"))
        {
            if (m_debugEnabled)
            {
                qDebug() << "[UsMap] style found:" << style;
            }
            return stylePart.mid(position + 1);
        }
    }
    return {};
}

void UsMap::processStateElement(const QXmlStreamAttributes& attributes)
{
    State state;
    state.id   = attributes.value("id").toString();
    state.name = attributes.value("data-name").toString();

    if (state.id.isEmpty() || state.name.isEmpty())
    {
        return;
    }

    if (auto rgb = parseHexColor(extractFill(attributes)))
    {
        state.fill = *rgb;
    }

    insertOrUpdateState(std::move(state));
}

void UsMap::insertOrUpdateState(State&& state)
{
    if (m_stateIdToIndex.contains(state.id))
    {
        std::size_t idx = static_cast<std::size_t>(m_stateIdToIndex.value(state.id));
        if (m_states[idx].name.isEmpty() && !state.name.isEmpty())
        {
            m_states[idx].name = state.name;
        }
        return;
    }

    int idx = static_cast<int>(m_states.size());
    m_stateIdToIndex.insert(state.id, idx);
    m_states.push_back(std::move(state));
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
        {
            continue;
        }

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

    m_debugDir =
        dumpDir.isEmpty() ? QCoreApplication::applicationDirPath() + "/usmap_debug" : dumpDir;

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

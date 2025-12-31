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

    if (m_debugEnabled and m_debugDir.isEmpty())
    {
        setDebug(m_debugEnabled);
    }
}

bool UsMap::loadAndParseSvg(QString* errorMessage)
{
    if (not loadSvgPatched(errorMessage))
    {
        setError(errorMessage, "[UsMap] QSvgRenderer: failed to load " + m_svgFilePath);
        return false;
    }

    if (not scanStatesFromSvg())
    {
        qWarning() << "[UsMap] No states parsed from SVG.";
    }

    if (not loadSvgOutlineWithoutFill(errorMessage))
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

    if (not m_svgOutlineRenderer.isValid())
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

QString UsMap::rgbToHex(QRgb c) const
{
    const int rgb = (qRed(c) << 16) bitor (qGreen(c) << 8) bitor qBlue(c);
    return QStringLiteral("#%1").arg(rgb, 6, 16, QChar('0'));
}

void UsMap::buildColorToStateMap()
{
    m_colorToState.clear();

    for (int stateId = 0; stateId < static_cast<int>(m_states.size()); ++stateId)
    {
        const auto& state = m_states[static_cast<std::size_t>(stateId)];

        QRgb svgColor = state.fill;

        if (svgColor == 0)
        {
            qWarning() << "[UsMap] No fill color for state:" << state.id;
            continue;
        }

        m_colorToState.insert(svgColor, static_cast<uint8_t>(stateId));

        if (m_debugEnabled)
        {
            qDebug() << "[UsMap] State:" << state.id << "| Fill color:" << rgbToHex(svgColor);
        }
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
        const QRgb*           linePtr = reinterpret_cast<const QRgb*>(colorImage.constScanLine(y));
        std::span<const QRgb> line{linePtr, static_cast<std::size_t>(cols)};

        for (int x = 0; x < cols; ++x)
        {
            const int i = y * cols + x;
            if (not m_outputProducts.activeStates[static_cast<std::size_t>(i)])
            {
                continue;
            }
            if (qAlpha(line[static_cast<std::size_t>(x)]) == 0)
            {
                continue;
            }

            auto it = m_colorToState.constFind(line[static_cast<std::size_t>(x)]);
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
    if (not loadAndParseSvg(errorMessage))
    {
        return false;
    }

    const int cols = m_outputProducts.cols;
    const int rows = m_outputProducts.rows;

    buildMaskAndActiveStates(cols, rows);
    const QImage colorImage = renderColorImage(cols, rows);
    buildColorToStateMap();
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

    if (not mapFile.open(QIODevice::ReadOnly))
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
        if (not readSvgFile(errorMessage))
        {
            return false;
        }
    }

    const QByteArray statesWithoutFill = removeStatesFills(m_svgRawOriginal);

    if (not m_svgOutlineRenderer.load(statesWithoutFill))
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
    if (not m_svgRenderer.load(m_svgRaw))
    {
        setError(errorMessage, "[UsMap] QSvgRenderer: failed to load patched svg");
        return false;
    }

    return true;
}

bool UsMap::loadSvgPatched(QString* errorMessage)
{
    if (not readSvgFile(errorMessage))
    {
        return false;
    }

    removeStatesOutlines();

    if (not loadSvgIntoRenderer(errorMessage))
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
    if (not isValidHexColor(hex))
    {
        return std::nullopt;
    }
    bool ok  = false;
    uint val = hex.mid(1).toUInt(&ok, 16);
    if (not ok)
    {
        return std::nullopt;
    }

    int r = (val >> 16) bitand 0xFF;
    int g = (val >> 8) bitand 0xFF;
    int b = (val) bitand 0xFF;
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
        if (position > 0 and stylePart.left(position) == QLatin1StringView("fill"))
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

    if (state.id.isEmpty() or state.name.isEmpty())
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
        if (m_states[idx].name.isEmpty() and not state.name.isEmpty())
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

    while (not xml.atEnd())
    {
        xml.readNext();

        if (not xml.isStartElement())
        {
            continue;
        }

        const QString tagName = xml.name().toString();
        if (not isStateElement(tagName))
        {
            continue;
        }

        processStateElement(xml.attributes());
    }

    reportXmlError(xml);
    return not m_states.empty();
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

    if (not m_debugEnabled)
    {
        return;
    }

    QDir().mkpath(m_debugDir);
    qDebug() << "[UsMap] Debug enabled. Dump dir:" << m_debugDir;
}

void UsMap::debugSave(const QString& name, const QImage& img) const
{
    if (not m_debugEnabled or name.isEmpty())
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
    return stateId not_eq kNoState and stateId < m_states.size();
}

QString UsMap::getStateName(uint8_t stateId) const
{
    if (not isValidStateId(stateId))
    {
        return {};
    }
    const auto& st = m_states[stateId];
    return st.name.isEmpty() ? st.id : st.name;
}

QString UsMap::getStateId(uint8_t stateId) const
{
    if (not isValidStateId(stateId))
    {
        return {};
    }
    return m_states[stateId].id;
}

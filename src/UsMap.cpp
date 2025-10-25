#include "UsMap.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QtMath>
#include <cstdint>
#include <qregularexpression.h>

const std::array<const char* const, 51> UsMap::m_stateIDs = {
    "AL", "AK", "AZ", "AR", "CA", "CO", "CT", "DE", "FL", "GA", "HI", "ID", "IL",
    "IN", "IA", "KS", "KY", "LA", "ME", "MD", "MA", "MI", "MN", "MS", "MO", "MT",
    "NE", "NV", "NH", "NJ", "NM", "NY", "NC", "ND", "OH", "OK", "OR", "PA", "RI",
    "SC", "SD", "TN", "TX", "UT", "VT", "VA", "WA", "WV", "WI", "WY", "DC"};

UsMap::UsMap(QString svgFilePath, int cols, int rows)
    : m_svgFilePath(std::move(svgFilePath)), m_outputProducts({{}, {}, cols, rows})
{
    m_outputProducts.activeStates.resize(
        static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows), 0);
    m_outputProducts.stateIds.resize(
        static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows), kNoState);
    m_statePixelCount.assign(m_stateIDs.size(), 0);
}

bool UsMap::buildProducts(QString* errorMessage)
{
    if (!loadSvgPatched(errorMessage))
    {
        if (errorMessage)
            *errorMessage = "QSvgRenderer: failed to load " + m_svgFilePath;
        return false;
    }

    const int cols = m_outputProducts.cols, rows = m_outputProducts.rows;
    std::fill(m_statePixelCount.begin(), m_statePixelCount.end(), 0);
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
            m_outputProducts.activeStates[y * cols + x] = (line[x] > 0) ? 1 : 0;
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

    for (int sid = 0; sid < (int)m_stateIDs.size(); ++sid)
    {
        const QString elemId = QString::fromLatin1(m_stateIDs[sid]);
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
            debugSave(QString("state_%1.png").arg(elemId), oneState);

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

        if (m_colorToState.contains(best) && m_colorToState.value(best) != uint8_t(sid))
        {
            qWarning() << "[UsMap] COLOR CONFLICT"
                       << QString("#%1").arg((qRed(best) << 16) | (qGreen(best) << 8) | qBlue(best),
                                             6, 16, QChar('0'))
                       << "between" << m_stateIDs[m_colorToState.value(best)] << "and" << elemId
                       << "(ensure unique fills in SVG)";
        }
        else
        {
            m_colorToState.insert(best, uint8_t(sid));
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
            if (!m_outputProducts.activeStates[i])
                continue;
            if (qAlpha(cl[x]) == 0)
                continue;

            const QRgb key = quant(cl[x]);
            auto       it  = m_colorToState.constFind(key);
            if (it != m_colorToState.constEnd())
            {
                const uint8_t sid            = it.value();
                m_outputProducts.stateIds[i] = sid;
                m_statePixelCount[sid] += 1;
            }
        }
    }

    m_productsBuilt = true;
    return true;
}

bool UsMap::loadSvgPatched(QString* errorMessage)
{
    QFile f(m_svgFilePath);
    if (!f.open(QIODevice::ReadOnly))
    {
        if (errorMessage)
            *errorMessage = "Cannot open " + m_svgFilePath;
        return false;
    }
    m_svgRaw = f.readAll();
    f.close();

    m_svgRaw.replace("stroke:#000000", "stroke:none");
    m_svgRaw.replace("stroke:#000", "stroke:none");
    m_svgRaw.replace("stroke:black", "stroke:none");

    if (!m_svgRenderer.load(m_svgRaw))
    {
        if (errorMessage)
            *errorMessage = "QSvgRenderer: failed to load patched svg";
        return false;
    }

    if (!buildColorMapFromSvg())
    {
        if (errorMessage)
        {
            *errorMessage = "Failed to extract fill colors from SVG";
        }
        qWarning() << "[UsMap] No color->state map built. Will not assign by color.";
    }
    return true;
}

std::optional<QRgb> UsMap::parseHexColor(const QString& hex)
{
    if (hex.size() != 7 || !hex.startsWith('#'))
        return std::nullopt;
    bool ok  = false;
    uint val = hex.mid(1).toUInt(&ok, 16);
    if (!ok)
        return std::nullopt;
    int r = (val >> 16) & 0xFF;
    int g = (val >> 8) & 0xFF;
    int b = (val) & 0xFF;
    return qRgb(r, g, b);
}

bool UsMap::buildColorMapFromSvg()
{
    m_colorToState.clear();
    const QString svg = QString::fromUtf8(m_svgRaw);

    for (int sid = 0; sid < (int)m_stateIDs.size(); ++sid)
    {
        const QString id = QString::fromLatin1(m_stateIDs[sid]);

        QRegularExpression reTag(
            QStringLiteral(R"(id\s*=\s*"%1"[^>]*>)").arg(QRegularExpression::escape(id)),
            QRegularExpression::DotMatchesEverythingOption |
                QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = reTag.match(svg);
        if (!m.hasMatch())
        {
            qWarning() << "[UsMap] id not found in SVG:" << id;
            continue;
        }
        const int tagEnd   = m.capturedEnd();
        int       tagStart = svg.lastIndexOf('<', m.capturedStart());
        if (tagStart < 0 || tagEnd <= tagStart)
        {
            qWarning() << "[UsMap] malformed tag for id:" << id;
            continue;
        }
        const QString tag = svg.mid(tagStart, tagEnd - tagStart);

        QRegularExpression      reFill("fill\\s*=\\s*\"(#[0-9a-fA-F]{6})\"");
        QRegularExpressionMatch mf = reFill.match(tag);
        QString                 hex;
        if (mf.hasMatch())
        {
            hex = mf.captured(1);
        }
        else
        {
            QRegularExpression      reStyle("style\\s*=\\s*\"([^\"]*)\"");
            QRegularExpressionMatch ms = reStyle.match(tag);
            if (ms.hasMatch())
            {
                const QString           style = ms.captured(1);
                QRegularExpression      reFillInStyle(";?\\s*fill\\s*:\\s*(#[0-9a-fA-F]{6})");
                QRegularExpressionMatch mf2 = reFillInStyle.match(style);
                if (mf2.hasMatch())
                    hex = mf2.captured(1);
            }
        }

        if (hex.isEmpty())
        {
            qWarning() << "[UsMap] fill not found for id:" << id;
            continue;
        }
        if (auto rgb = parseHexColor(hex))
        {
            const QRgb key = *rgb;
            m_colorToState.insert(key, static_cast<uint8_t>(sid));
            qDebug() << "[UsMap] color map:" << id << hex;
        }
        else
        {
            qWarning() << "[UsMap] bad color for id:" << id << hex;
        }
    }

    QHash<QRgb, QVector<QString>> colorToIds;
    for (auto it = colorToIds.constBegin(); it != colorToIds.constEnd(); ++it)
    {
        if (it.value().size() > 1)
        {
            qWarning().noquote() << "[UsMap] DUP color"
                                 << QString("#%1").arg((qRed(it.key()) << 16) |
                                                           (qGreen(it.key()) << 8) |
                                                           qBlue(it.key()),
                                                       6, 16, QChar('0'))
                                 << "for states:" << it.value().join(',');
        }
    }

    return !m_colorToState.isEmpty();
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
        return kNoState;
    const float sx = float(m_outputProducts.cols) / float(viewSize.width());
    const float sy = float(m_outputProducts.rows) / float(viewSize.height());
    int         x  = int(position.x() * sx);
    int         y  = int(position.y() * sy);
    if (x < 0 || y < 0 || x >= m_outputProducts.cols || y >= m_outputProducts.rows)
        return kNoState;
    return m_outputProducts.stateIds[y * m_outputProducts.cols + x];
}

std::optional<int> UsMap::indexOfAbbrev(QString twoLetterAbbrev)
{
    QString abbrev = twoLetterAbbrev.trimmed().toUpper();
    for (int i = 0; i < (int)m_stateIDs.size(); ++i)
    {
        if (abbrev == QLatin1String(m_stateIDs[i]))
            return i;
    }
    return std::nullopt;
}

QRectF UsMap::svgViewBox() const
{
    return m_svgRenderer.viewBoxF();
}

UsMap::Products& UsMap::getProducts() const
{
    return const_cast<UsMap::Products&>(m_outputProducts);
}

std::span<const char* const> UsMap::abbrevs()
{
    return std::span<const char* const>(m_stateIDs.data(), m_stateIDs.size());
}

void UsMap::setDebug(bool on, QString dumpDir)
{
    m_debugEnabled = on;
    m_debugDir     = dumpDir.isEmpty() ? QDir::tempPath() + "/usmap_debug" : dumpDir;
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
        return;
    const QString path = m_debugDir + "/" + name;
    img.save(path);
    qDebug() << "[UsMap] saved" << path;
}

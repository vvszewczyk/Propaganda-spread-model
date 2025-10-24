#include "UsMap.hpp"

#include <QPainter>
#include <cstdint>
#include <qdir.h>
#include <qobject.h>
#include <qpainter.h>
#include <qstringliteral.h>
#include <qwindowdefs.h>

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
            m_outputProducts.activeStates[y * cols + x] = (line[x] > 0) ? 1 : 0;
        }
    }

    m_layerImage = QImage(cols, rows, QImage::Format_ARGB32_Premultiplied);
    for (int sid = 0; sid < (int)m_stateIDs.size(); ++sid)
    {
        const QString elemId = QString::fromLatin1(m_stateIDs[sid]);
        if (!m_svgRenderer.elementExists(elemId))
        {
            continue;
        }
        m_layerImage.fill(Qt::transparent);
        {
            QPainter pl(&m_layerImage);
            pl.setRenderHint(QPainter::Antialiasing, false);
            pl.setCompositionMode(QPainter::CompositionMode_Source);

            const QRectF vb = m_svgRenderer.viewBoxF();
            const qreal  sx = qreal(cols) / vb.width();
            const qreal  sy = qreal(rows) / vb.height();
            pl.scale(sx, sy);

            const QRectF bbox = m_svgRenderer.boundsOnElement(elemId);
            if (!bbox.isValid() || bbox.isEmpty())
                continue;
            pl.setClipRect(bbox);

            m_svgRenderer.render(&pl, elemId);
        }

        for (int y = 0; y < rows; ++y)
        {
            const QRgb* line = reinterpret_cast<const QRgb*>(m_layerImage.constScanLine(y));
            for (int x = 0; x < cols; ++x)
            {
                const int i = y * cols + x;
                if (!m_outputProducts.activeStates[i])
                    continue;
                if (qAlpha(line[x]) > 0 && m_outputProducts.stateIds[i] == kNoState)
                    m_outputProducts.stateIds[i] = static_cast<uint8_t>(sid);
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
    QByteArray svg = f.readAll();
    f.close();
    svg.replace("stroke:#000", "stroke:none");

    if (!m_svgRenderer.load(svg))
    {
        if (errorMessage)
            *errorMessage = "QSvgRenderer: failed to load patched svg";
        return false;
    }
    return true;
}

void UsMap::drawBackground(QPainter& painter, const QRect& rect) const
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    m_svgRenderer.render(&painter, QStringLiteral("states"), rect);
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

#include "GridWidget.hpp"

#include "Constants.hpp"
#include "Simulation.hpp"

#include <QColor>
#include <QCursor>
#include <QPainter>
#include <QPoint>
#include <QStringLiteral>
#include <QTimer>
#include <QToolTip>
#include <QWheelEvent>
#include <QWidget>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <qnamespace.h>
#include <qtypes.h>
#include <sys/stat.h>

GridWidget::GridWidget(QWidget* parent) : QWidget{parent}
{
}

void GridWidget::setSimulation(const Simulation* simulation) noexcept
{
    this->m_sim = simulation;
    update();
}

void GridWidget::setUsMap(const UsMap* usMap) noexcept
{
    this->m_usMap = usMap;
    update();
}

void GridWidget::setShowGrid(bool on) noexcept
{
    this->m_showGrid = on;
    update();
}

QColor GridWidget::getColorFor(CellData cell) const noexcept
{
    switch (cell.state)
    {
    case State::S:
        return Qt::white;
    case State::E:
        return QColor(255, 200, 0);
    case State::I:
        return Qt::red;
    case State::R:
        return QColor(0, 120, 215);
    case State::D:
        return Qt::darkGray;
    default:
        return Qt::black;
    }
}

void GridWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(144, 213, 255));

    if (!m_usMap || !m_sim)
    {
        return;
    }

    const QRectF destRectF = mapDestRect();
    if (!destRectF.isValid())
    {
        return;
    }

    const QRect destRect = destRectF.toAlignedRect();

    rebuildCellsImage();

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    const auto& products = m_usMap->getProducts();
    painter.drawImage(destRect, m_cellsImage, QRectF(0, 0, products.cols, products.rows));

    m_usMap->drawStateOutlines(painter, destRect);

    if (m_showGrid)
    {
        drawGrid(painter);
    }

    drawOuterFrame(painter);
}

void GridWidget::drawGrid(QPainter& painter) const
{
    if (!m_usMap || !m_showGrid)
    {
        return;
    }

    const auto& products = m_usMap->getProducts();
    if (products.cols <= 0 || products.rows <= 0)
    {
        return;
    }

    float cellWidth, cellHeight, mapWidth, mapHeight, originXCenter, originYCenter;
    computeGeometry(m_zoom, cellWidth, cellHeight, mapWidth, mapHeight, originXCenter,
                    originYCenter);

    const float originX = originXCenter + static_cast<float>(m_pan.x());
    const float originY = originYCenter + static_cast<float>(m_pan.y());

    QPen pen(Qt::white);
    pen.setColor(QColor(255, 255, 255, 40));
    pen.setWidth(1);
    pen.setCosmetic(true);
    painter.setPen(pen);

    for (int x = 1; x < products.cols; ++x)
    {
        const qreal xpos =
            static_cast<qreal>(originX) + static_cast<qreal>(x) * static_cast<qreal>(cellWidth);
        painter.drawLine(
            QPointF{xpos, static_cast<qreal>(originY)},
            QPointF{xpos, static_cast<qreal>(originY) + static_cast<qreal>(mapHeight)});
    }
    for (int y = 1; y < products.rows; ++y)
    {
        const qreal ypos =
            static_cast<qreal>(originY) + static_cast<qreal>(y) * static_cast<qreal>(cellHeight);
        painter.drawLine(QPointF{static_cast<qreal>(originX), ypos},
                         QPointF{static_cast<qreal>(originX) + static_cast<qreal>(mapWidth), ypos});
    }
}

void GridWidget::drawOuterFrame(QPainter& painter) const
{
    const int frameThickness = 1;

    QPen framePen(Qt::black);
    framePen.setWidth(frameThickness);
    framePen.setJoinStyle(Qt::MiterJoin);
    framePen.setCosmetic(true);
    painter.setPen(framePen);
    painter.setBrush(Qt::NoBrush);

    QRectF frameRect = this->rect();
    frameRect.adjust(0.5, 0.5, -1.5, -1.5);

    painter.drawRect(frameRect);
}

void GridWidget::computeGeometry(qreal  zoom,
                                 float& cellWidth,
                                 float& cellHeight,
                                 float& mapWidth,
                                 float& mapHeight,
                                 float& originXCenter,
                                 float& originYCenter) const
{
    const auto& products = m_usMap->getProducts();

    cellWidth =
        static_cast<float>(width()) / static_cast<float>(products.cols) * static_cast<float>(zoom);
    cellHeight =
        static_cast<float>(height()) / static_cast<float>(products.rows) * static_cast<float>(zoom);

    mapWidth  = cellWidth * static_cast<float>(products.cols);
    mapHeight = cellHeight * static_cast<float>(products.rows);

    originXCenter = 0.5f * (static_cast<float>(width()) - mapWidth);
    originYCenter = 0.5f * (static_cast<float>(height()) - mapHeight);
}

QRectF GridWidget::mapDestRect() const
{
    if (!m_usMap)
    {
        return {};
    }
    const auto& products = m_usMap->getProducts();
    if (products.cols <= 0 || products.rows <= 0)
        return QRectF();

    float cellWidth, cellHeight, mapWidth, mapHeight, originXCenter, originYCenter;
    computeGeometry(m_zoom, cellWidth, cellHeight, mapWidth, mapHeight, originXCenter,
                    originYCenter);

    const float originX = originXCenter + static_cast<float>(m_pan.x());
    const float originY = originYCenter + static_cast<float>(m_pan.y());

    return QRectF(static_cast<qreal>(originX), static_cast<qreal>(originY),
                  static_cast<qreal>(mapWidth), static_cast<qreal>(mapHeight));
}

void GridWidget::rebuildCellsImage() const
{
    if (!m_usMap || !m_sim)
    {
        return;
    }

    const auto& products = m_usMap->getProducts();
    if (products.cols <= 0 || products.rows <= 0)
    {
        return;
    }

    const QSize imgSize(products.cols, products.rows);
    if (m_cellsImage.size() != imgSize)
    {
        m_cellsImage = QImage(imgSize, QImage::Format_ARGB32_Premultiplied);
    }

    m_cellsImage.fill(Qt::transparent);

    for (int y = 0; y < products.rows; ++y)
    {
        for (int x = 0; x < products.cols; ++x)
        {
            const int index = y * products.cols + x;
            if (!products.activeStates[static_cast<std::size_t>(index)])
            {
                continue;
            }

            QColor color = getColorFor(m_sim->cell(x, y));

            const int stateId = products.stateIds[static_cast<std::size_t>(index)];
            if (m_coloredStates.contains(stateId))
            {
                color = m_coloredStates.value(stateId);
            }

            m_cellsImage.setPixelColor(x, y, color);
        }
    }
}

QPoint GridWidget::productPointFromWidgetPos(QPointF pos) const
{
    if (!m_usMap)
    {
        return QPoint{-1, -1};
    }

    const auto& products = m_usMap->getProducts();
    if (products.cols <= 0 || products.rows <= 0)
    {
        return QPoint{-1, -1};
    }

    const QRectF dest = mapDestRect();
    if (!dest.isValid())
    {
        return QPoint{-1, -1};
    }

    const float fx = static_cast<float>((pos.x() - dest.left()) / dest.width());
    const float fy = static_cast<float>((pos.y() - dest.top()) / dest.height());

    const int ix = static_cast<int>(std::floor(fx * static_cast<float>(products.cols)));
    const int iy = static_cast<int>(std::floor(fy * static_cast<float>(products.rows)));

    if (ix < 0 || iy < 0 || ix >= products.cols || iy >= products.rows)
    {
        return QPoint{-1, -1};
    }

    return QPoint{ix, iy};
}

uint8_t GridWidget::stateAtWidgetPos(QPointF pos) const
{
    if (!m_usMap)
    {
        return UsMap::kNoState;
    }

    const auto& products = m_usMap->getProducts();
    if (products.cols <= 0 || products.rows <= 0)
    {
        return UsMap::kNoState;
    }

    const QPoint cell = productPointFromWidgetPos(pos);
    if (cell.x() < 0 || cell.y() < 0)
    {
        return UsMap::kNoState;
    }

    const std::size_t index =
        static_cast<std::size_t>(cell.y()) * static_cast<std::size_t>(products.cols) +
        static_cast<std::size_t>(cell.x());

    return products.stateIds[index];
}

QString GridWidget::tooltipTextForState(uint8_t stateId) const
{
    if (!m_usMap)
        return {};

    const QString full = m_usMap->getStateName(stateId);
    const QString abbr = m_usMap->getStateId(stateId);

    if (full.isEmpty())
    {
        return abbr;
    }

    if (abbr.isEmpty() || full.compare(abbr, Qt::CaseInsensitive) == 0)
    {
        return full;
    }

    return QStringLiteral("%1 (%2)").arg(full, abbr);
}

void GridWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton)
    {
        m_isPanning  = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (!m_usMap)
    {
        return;
    }

    uint8_t sid = stateAtWidgetPos(event->pos());
    if (sid == UsMap::kNoState)
    {
        return;
    }

    int          stateId = static_cast<int>(sid);
    const QColor yellow(255, 230, 0);
    const QColor green(0, 255, 0);

    if (event->button() == Qt::LeftButton)
    {
        if (m_coloredStates.contains(stateId) && m_coloredStates.value(stateId) == yellow)
        {
            m_coloredStates.remove(stateId);
        }
        else
        {
            m_coloredStates.insert(stateId, yellow);
        }
    }
    else if (event->button() == Qt::RightButton)
    {
        if (m_coloredStates.contains(stateId) && m_coloredStates.value(stateId) == green)
        {
            m_coloredStates.remove(stateId);
        }
        else
        {
            m_coloredStates.insert(stateId, green);
        }
    }

    m_selectedSingleStateId = stateId;

    const QString txt = tooltipTextForState(sid);
    QToolTip::hideText();
    QToolTip::showText(event->globalPosition().toPoint(), txt, this, QRect(), 3000);
    update();
}

void GridWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isPanning)
    {
        const QPoint delta = event->pos() - m_lastPanPos;
        m_lastPanPos       = event->pos();

        m_pan.rx() += delta.x();
        m_pan.ry() += delta.y();
        update();
        event->accept();
        return;
    }

    if (!m_usMap)
        return;

    const uint8_t sid      = stateAtWidgetPos(event->pos());
    const int     newHover = (sid == UsMap::kNoState) ? -1 : int(sid);

    if (newHover == m_hoverSid)
        return;

    m_hoverSid = newHover;

    if (sid != UsMap::kNoState)
    {
        const QString txt = tooltipTextForState(sid);
        QToolTip::hideText();
        QToolTip::showText(event->globalPosition().toPoint(), txt, this, QRect(), 3000);
    }
    else
    {
        QToolTip::hideText();
    }
}

void GridWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event)
    m_hoverSid = -1;
    QToolTip::hideText();
}

void GridWidget::wheelEvent(QWheelEvent* event)
{
    if (!(event->modifiers() & Qt::ControlModifier))
    {
        QWidget::wheelEvent(event);
        return;
    }

    if (!m_usMap)
    {
        event->ignore();
        return;
    }

    const int delta = event->angleDelta().y();
    if (delta == 0)
    {
        event->ignore();
        return;
    }

    const QPointF cursorPos = event->position();

    const auto& products = m_usMap->getProducts();
    if (products.cols <= 0 || products.rows <= 0)
    {
        event->ignore();
        return;
    }

    float cellWidthBefore, cellHeightBefore, mapWidthBefore, mapHeightBefore, originXCenterBefore,
        originYCenterBefore;

    computeGeometry(m_zoom, cellWidthBefore, cellHeightBefore, mapWidthBefore, mapHeightBefore,
                    originXCenterBefore, originYCenterBefore);

    const float originXBefore = originXCenterBefore + static_cast<float>(m_pan.x());
    const float originYBefore = originYCenterBefore + static_cast<float>(m_pan.y());

    const qreal mapX =
        (cursorPos.x() - static_cast<qreal>(originXBefore)) / static_cast<qreal>(cellWidthBefore);
    const float mapY = (static_cast<float>(cursorPos.y()) - originYBefore) / cellHeightBefore;

    constexpr qreal zoomStep = 1.1;
    qreal           newZoom  = m_zoom * ((delta > 0) ? zoomStep : 1.0 / zoomStep);
    newZoom                  = std::clamp(newZoom, 0.2, 5.0);

    if (std::abs(newZoom - m_zoom) < 0.0001)
    {
        event->ignore();
        return;
    }

    m_zoom = newZoom;

    float cellWidthAfter, cellHeightAfter, mapWidthAfter, mapHeightAfter;
    float originXCenterAfter, originYCenterAfter;
    computeGeometry(m_zoom, cellWidthAfter, cellHeightAfter, mapWidthAfter, mapHeightAfter,
                    originXCenterAfter, originYCenterAfter);

    m_pan.setX(cursorPos.x() - static_cast<qreal>(originXCenterAfter) -
               mapX * static_cast<qreal>(cellWidthAfter));

    m_pan.setY(cursorPos.y() - static_cast<qreal>(originYCenterAfter) -
               static_cast<qreal>(mapY) * static_cast<qreal>(cellHeightAfter));

    update();
    event->accept();
}

void GridWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_isPanning && event->button() == Qt::MiddleButton)
    {
        m_isPanning = false;
        unsetCursor();
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

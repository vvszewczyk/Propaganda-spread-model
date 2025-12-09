#include "GridWidget.hpp"

#include "Constants.hpp"
#include "Simulation.hpp"
#include "UsMap.hpp"

#include <QColor>
#include <QCursor>
#include <QObject>
#include <QPainter>
#include <QPoint>
#include <QStringLiteral>
#include <QStringView>
#include <QTimer>
#include <QToolTip>
#include <QWheelEvent>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <cstdint>

using namespace app::ui;

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

void GridWidget::clearMap() noexcept
{
    m_coloredCells.clear();
    m_coloredStates.clear();
    m_selectedStateIds.clear();
    m_selectedSingleStateId = -1;
    m_hoverSid              = -1;
    update();
}

void GridWidget::resetView() noexcept
{
    m_zoom = 1.0;
    m_pan  = QPointF{0.0, 0.0};
    emit zoomChanged(m_zoom);
    emit cellInfoChanged(QString());
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

    const Geometry g = computeGeometry(m_zoom);

    const float originX = g.originXCenter + static_cast<float>(m_pan.x());
    const float originY = g.originYCenter + static_cast<float>(m_pan.y());

    QPen pen(Qt::white);
    pen.setColor(QColor(255, 255, 255, Config::GridWidget::gridAlpha));
    pen.setWidth(1);
    pen.setCosmetic(true);
    painter.setPen(pen);

    for (int x = 1; x < products.cols; ++x)
    {
        const qreal xpos =
            static_cast<qreal>(originX) + static_cast<qreal>(x) * static_cast<qreal>(g.cellWidth);
        painter.drawLine(
            QPointF{xpos, static_cast<qreal>(originY)},
            QPointF{xpos, static_cast<qreal>(originY) + static_cast<qreal>(g.mapHeight)});
    }
    for (int y = 1; y < products.rows; ++y)
    {
        const qreal ypos =
            static_cast<qreal>(originY) + static_cast<qreal>(y) * static_cast<qreal>(g.cellHeight);
        painter.drawLine(
            QPointF{static_cast<qreal>(originX), ypos},
            QPointF{static_cast<qreal>(originX) + static_cast<qreal>(g.mapWidth), ypos});
    }
}

bool GridWidget::canPaint() const noexcept
{
    return m_usMap and m_sim;
}

void GridWidget::drawCells(QPainter& painter, const QRectF& destRectF) const
{
    const auto& products = m_usMap->getProducts();
    const QRect destRect = destRectF.toAlignedRect();
    painter.drawImage(destRect, m_cellsImage, QRectF(0, 0, products.cols, products.rows));
}

void GridWidget::drawOutlines(QPainter& painter, const QRectF& destRectF) const
{
    m_usMap->drawStateOutlines(painter, destRectF.toAlignedRect());
}

void GridWidget::drawOuterFrame(QPainter& painter) const
{
    QPen framePen(Qt::black);
    framePen.setWidth(Config::GridWidget::frameThickness);
    framePen.setJoinStyle(Qt::MiterJoin);
    framePen.setCosmetic(true);
    painter.setPen(framePen);
    painter.setBrush(Qt::NoBrush);

    QRectF frameRect = this->rect();
    frameRect.adjust(0.5, 0.5, -1.5, -1.5);

    painter.drawRect(frameRect);
}

void GridWidget::applyBrushAt(const QPointF& pos, Qt::MouseButtons buttons)
{
    if (!m_usMap)
    {
        return;
    }

    const uint8_t sid = stateAtWidgetPos(pos);
    if (sid == UsMap::kNoState)
    {
        return;
    }

    const QPoint cell = productPointFromWidgetPos(pos);
    if (cell.x() < 0 || cell.y() < 0)
    {
        return;
    }

    const int    stateId = static_cast<int>(sid);
    const QColor red(255, 0, 0);
    const QColor green(0, 255, 0);

    if (buttons & Qt::LeftButton)
    {
        m_coloredCells.insert(cell, red);
    }

    if (buttons & Qt::RightButton)
    {
        m_coloredStates.insert(stateId, green);
    }

    update();
}

void GridWidget::updateCellInfoAt(const QPointF& position)
{
    if (!m_usMap || !m_sim)
    {
        emit cellInfoChanged(QString());
        return;
    }

    const uint8_t stateId = stateAtWidgetPos(position);
    if (stateId == UsMap::kNoState)
    {
        emit cellInfoChanged(QString());
        return;
    }

    const QPoint cell = productPointFromWidgetPos(position);
    if (cell.x() < 0 || cell.y() < 0)
    {
        emit cellInfoChanged(QString());
        return;
    }

    const auto cellData = m_sim->cell(cell.x(), cell.y());
    QString    stateStr;
    switch (cellData.state)
    {
    case State::S:
        stateStr = QStringLiteral("S");
        break;
    case State::E:
        stateStr = QStringLiteral("E");
        break;
    case State::I:
        stateStr = QStringLiteral("I");
        break;
    case State::R:
        stateStr = QStringLiteral("R");
        break;
    case State::D:
        stateStr = QStringLiteral("D");
        break;
    default:
        stateStr = QStringLiteral("?");
        break;
    }

    const QString usState = tooltipTextForState(stateId);

    const QString info = QStringLiteral("Cell (%1, %2):\nState=%3\n(%4)\nResilience=%5\nFatigue=%6")
                             .arg(cell.x())
                             .arg(cell.y())
                             .arg(stateStr)
                             .arg(usState)
                             .arg(cellData.resilience, 0, 'f', 2)
                             .arg(cellData.fatigue, 0, 'f', 2);

    emit cellInfoChanged(info);
}

GridWidget::Geometry GridWidget::computeGeometry(qreal zoom) const
{
    const auto& products = m_usMap->getProducts();

    Geometry g{};
    g.cellWidth =
        static_cast<float>(width()) / static_cast<float>(products.cols) * static_cast<float>(zoom);
    g.cellHeight =
        static_cast<float>(height()) / static_cast<float>(products.rows) * static_cast<float>(zoom);

    g.mapWidth  = g.cellWidth * static_cast<float>(products.cols);
    g.mapHeight = g.cellHeight * static_cast<float>(products.rows);

    g.originXCenter = 0.5f * (static_cast<float>(width()) - g.mapWidth);
    g.originYCenter = 0.5f * (static_cast<float>(height()) - g.mapHeight);

    return g;
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

    const Geometry g = computeGeometry(m_zoom);

    const float originX = g.originXCenter + static_cast<float>(m_pan.x());
    const float originY = g.originYCenter + static_cast<float>(m_pan.y());

    return QRectF(static_cast<qreal>(originX), static_cast<qreal>(originY),
                  static_cast<qreal>(g.mapWidth), static_cast<qreal>(g.mapHeight));
}

void GridWidget::rebuildCellsImageIfNeeded() const
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

            const int    stateId = products.stateIds[static_cast<std::size_t>(index)];
            const QPoint cellPoint{x, y};

            if (m_coloredCells.contains(cellPoint))
            {
                color = m_coloredCells.value(cellPoint);
            }
            else if (m_coloredStates.contains(stateId))
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

void GridWidget::updatePanForZoom(const QPointF& cursorPos, qreal newZoom)
{
    const Geometry before = computeGeometry(m_zoom);

    const float originXBefore = before.originXCenter + static_cast<float>(m_pan.x());
    const float originYBefore = before.originYCenter + static_cast<float>(m_pan.y());

    const qreal mapX =
        (cursorPos.x() - static_cast<qreal>(originXBefore)) / static_cast<qreal>(before.cellWidth);
    const qreal mapY =
        (cursorPos.y() - static_cast<qreal>(originYBefore)) / static_cast<qreal>(before.cellHeight);

    m_zoom = newZoom;
    emit zoomChanged(m_zoom);

    const Geometry after = computeGeometry(m_zoom);

    m_pan.setX(cursorPos.x() - static_cast<qreal>(after.originXCenter) -
               mapX * static_cast<qreal>(after.cellWidth));
    m_pan.setY(cursorPos.y() - static_cast<qreal>(after.originYCenter) -
               mapY * static_cast<qreal>(after.cellHeight));
}

void GridWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), Config::GridWidget::backgroundColor);

    if (not canPaint())
    {
        return;
    }

    const QRectF destRectF = mapDestRect();
    if (!destRectF.isValid())
    {
        return;
    }

    rebuildCellsImageIfNeeded();

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    drawCells(painter, destRectF);
    drawOutlines(painter, destRectF);

    if (m_showGrid)
    {
        drawGrid(painter);
    }

    drawOuterFrame(painter);
}

void GridWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton)
    {
        m_isPanning  = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        return event->accept();
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
    const QColor red(255, 0, 0);
    const QColor green(0, 255, 0);
    const QPoint cell = productPointFromWidgetPos(event->pos());

    if (cell.x() < 0 || cell.y() < 0)
    {
        return;
    }

    if (event->button() == Qt::LeftButton)
    {
        if (m_coloredCells.contains(cell) && m_coloredCells.value(cell) == red)
        {
            m_coloredCells.remove(cell);
        }
        else
        {
            m_coloredCells.insert(cell, red);
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
    QToolTip::showText(event->globalPosition().toPoint(), txt, this, QRect(),
                       Config::GridWidget::tooltopMs);
    updateCellInfoAt(event->position());
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
        return event->accept();
    }

    if (event->buttons() & (Qt::LeftButton | Qt::RightButton))
    {
        applyBrushAt(event->position(), event->buttons());
        return event->accept();
    }

    if (!m_usMap)
    {
        return;
    }

    const uint8_t sid      = stateAtWidgetPos(event->pos());
    const int     newHover = (sid == UsMap::kNoState) ? -1 : int(sid);

    if (newHover == m_hoverSid)
    {
        updateCellInfoAt(event->position());
        return;
    }

    m_hoverSid = newHover;

    if (sid != UsMap::kNoState)
    {
        const QString txt = tooltipTextForState(sid);
        QToolTip::hideText();
        QToolTip::showText(event->globalPosition().toPoint(), txt, this, QRect(),
                           Config::GridWidget::tooltopMs);

        updateCellInfoAt(event->position());
    }
    else
    {
        QToolTip::hideText();
        emit cellInfoChanged(QString());
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
        return QWidget::wheelEvent(event);
    }

    if (!m_usMap)
    {
        return event->ignore();
    }

    const auto& products = m_usMap->getProducts();
    if (products.cols <= 0 || products.rows <= 0)
    {
        return event->ignore();
    }

    const int delta = event->angleDelta().y();
    if (delta == 0)
    {
        return event->ignore();
    }

    qreal newZoom =
        m_zoom * ((delta > 0) ? Config::GridWidget::zoomStep : 1.0 / Config::GridWidget::zoomStep);
    newZoom = std::clamp(newZoom, Config::GridWidget::zoomMin, Config::GridWidget::zoomMax);

    if (std::abs(newZoom - m_zoom) < 0.0001)
    {
        return event->ignore();
    }

    updatePanForZoom(event->position(), newZoom);

    update();
    event->accept();
}

void GridWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_isPanning && event->button() == Qt::MiddleButton)
    {
        m_isPanning = false;
        unsetCursor();
        return event->accept();
    }

    QWidget::mouseReleaseEvent(event);
}

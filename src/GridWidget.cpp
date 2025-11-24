#include "GridWidget.hpp"

#include "Constants.hpp"
#include "Simulation.hpp"

#include <QPainter>
#include <QStringLiteral>
#include <QToolTip>
#include <qcolor.h>
#include <qpainter.h>
#include <qtypes.h>

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
    painter.fillRect(rect(), Qt::black);

    if (!m_usMap)
    {
        return;
    }

    if (m_sim)
    {
        drawCells(painter);
    }

    if (m_showGrid)
    {
        drawGrid(painter);
    }
}

void GridWidget::drawCells(QPainter& painter) const
{
    const auto& products = m_usMap->getProducts();
    if (products.cols <= 0 || products.rows <= 0)
    {
        return;
    }

    const float cellWidth  = static_cast<float>(width()) / static_cast<float>(products.cols);
    const float cellHeight = static_cast<float>(height()) / static_cast<float>(products.rows);

    for (int y = 0; y < products.rows; ++y)
    {
        for (int x = 0; x < products.cols; ++x)
        {
            const int index = y * products.cols + x;
            if (!products.activeStates[static_cast<std::size_t>(index)])
            {
                continue;
            }

            const auto& cell  = m_sim->cell(x, y);
            QColor      color = getColorFor(cell);
            QRectF      rectangleToDraw{x * cellWidth, y * cellHeight, cellWidth, cellHeight};
            painter.fillRect(rectangleToDraw, color);
        }
    }

    if (!m_coloredStates.isEmpty())
    {
        for (int y = 0; y < products.rows; ++y)
        {
            for (int x = 0; x < products.cols; ++x)
            {
                const int index = y * products.cols + x;
                if (!products.activeStates[static_cast<std::size_t>(index)])
                {
                    continue;
                }

                const int stateId = products.stateIds[static_cast<std::size_t>(index)];
                if (m_coloredStates.contains(stateId))
                {
                    QRectF rectangleToDraw{x * cellWidth, y * cellHeight, cellWidth, cellHeight};
                    painter.fillRect(rectangleToDraw, m_coloredStates.value(stateId));
                }
            }
        }
    }
}

void GridWidget::drawGrid(QPainter& painter) const
{
    const auto& products = m_usMap->getProducts();
    if (products.cols <= 0 || products.rows <= 0)
    {
        return;
    }

    const float cellWidth  = static_cast<float>(width()) / static_cast<float>(products.cols);
    const float cellHeight = static_cast<float>(height()) / static_cast<float>(products.rows);

    QPen pen(Qt::white);
    pen.setColor(QColor(255, 255, 255, 40));
    pen.setWidth(1);
    pen.setCosmetic(true);
    painter.setPen(pen);

    for (int x = 1; x < products.cols; ++x)
    {
        qreal xpos = x * cellWidth;
        painter.drawLine(QPointF{xpos, 0}, QPointF{xpos, qreal(height())});
    }
    for (int y = 1; y < products.rows; ++y)
    {
        qreal ypos = y * cellHeight;
        painter.drawLine(QPointF{0, ypos}, QPointF{qreal(width()), ypos});
    }
}

void GridWidget::mousePressEvent(QMouseEvent* event)
{
    if (!m_usMap)
    {
        return;
    }

    uint8_t sid = m_usMap->stateAtViewPos(event->pos(), size());
    if (sid == UsMap::kNoState)
    {
        return;
    }

    int          stateId = static_cast<int>(sid);
    const QColor yellow(255, 230, 0, 110);
    const QColor green(0, 255, 0, 110);

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
    const QString full      = m_usMap->getStateName(sid);
    const QString abbr      = m_usMap->getStateId(sid);
    const QString txt =
        full.compare(abbr, Qt::CaseInsensitive) == 0 ? abbr : QString("%1 (%2)").arg(full, abbr);

    setToolTip(txt);
    QToolTip::showText(event->globalPosition().toPoint(), txt, this, QRect(), 3000);
    update();
}

void GridWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_usMap)
        return;

    const auto sid      = m_usMap->stateAtViewPos(event->pos(), size());
    const int  newHover = (sid == UsMap::kNoState) ? -1 : int(sid);
    if (newHover == m_hoverSid)
    {
        return;
    }

    m_hoverSid = newHover;

    if (sid != UsMap::kNoState)
    {
        const QString stateId = m_usMap->getStateId(sid);
        const QString txt     = QStringLiteral("State: %1").arg(stateId);
        setToolTip(txt);
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

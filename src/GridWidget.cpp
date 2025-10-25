#include "GridWidget.hpp"

#include "Constants.hpp"
#include "Simulation.hpp"

#include <QPainter>
#include <qstringliteral.h>
#include <qtooltip.h>

GridWidget::GridWidget(QWidget* parent) : QWidget{parent}, m_sim{nullptr}, m_showGrid{true}
{
}

void GridWidget::setSimulation(const Simulation* simulation)
{
    this->m_sim = const_cast<Simulation*>(simulation);
}

void GridWidget::setUsMap(const UsMap* usMap)
{
    this->m_usMap = const_cast<UsMap*>(usMap);
}

void GridWidget::setShowGrid(bool value)
{
    this->m_showGrid = value;
    update();
}

void GridWidget::setEraseMode(bool value)
{
    m_eraseMode = value;
}

QColor GridWidget::getColorForCell(CellData cell) const
{
    if (cell.state == State::S)
    {
        return Qt::white;
    }
    else if (cell.state == State::I)
    {
        return Qt::red;
    }
    else if (cell.state == State::R)
    {
        return Qt::blue;
    }
    else
    {
        return Qt::green;
    }
}

void GridWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), Qt::black);

    if (m_usMap)
    {
        m_usMap->drawBackground(p, rect());
        const auto& prod = m_usMap->getProducts();
        const float cw   = float(width()) / float(prod.cols);
        const float ch   = float(height()) / float(prod.rows);

        if (m_sim)
        {
            for (int y = 0; y < prod.rows; ++y)
            {
                for (int x = 0; x < prod.cols; ++x)
                {
                    int i = y * prod.cols + x;
                    if (!prod.activeStates[i])
                        continue;

                    const auto& cell = m_sim->cell(x, y);
                    QColor      c    = getColorForCell(cell);
                    p.fillRect(QRectF(x * cw, y * ch, cw, ch), c);
                }
            }

            if (!m_coloredStates.isEmpty())
            {
                for (int y = 0; y < prod.rows; ++y)
                {
                    for (int x = 0; x < prod.cols; ++x)
                    {
                        int i = y * prod.cols + x;
                        if (!prod.activeStates[i])
                            continue;

                        int sid = prod.stateIds[i];
                        if (m_coloredStates.contains(sid))
                        {
                            p.fillRect(QRectF(x * cw, y * ch, cw, ch), m_coloredStates[sid]);
                        }
                    }
                }
            }
        }

        {
            QPainter::RenderHints oldHints = p.renderHints();
            p.setRenderHint(QPainter::Antialiasing, false);
            p.setRenderHint(QPainter::SmoothPixmapTransform, false);
            p.setRenderHints(oldHints);
        }
    }
}

void GridWidget::mousePressEvent(QMouseEvent* event)
{
    if (!m_usMap)
        return;

    uint8_t sid = m_usMap->stateAtViewPos(event->pos(), size());
    if (sid == UsMap::kNoState)
        return;

    int stateId = int(sid);

    if (event->button() == Qt::LeftButton)
    {
        // Lewy klik – żółty
        if (m_coloredStates.contains(stateId) &&
            m_coloredStates[stateId] == QColor(255, 230, 0, 110))
            m_coloredStates.remove(stateId); // ponowne kliknięcie usuwa kolor
        else
            m_coloredStates[stateId] = QColor(255, 230, 0, 110);
    }
    else if (event->button() == Qt::RightButton)
    {
        // Prawy klik – np. zielony
        if (m_coloredStates.contains(stateId) && m_coloredStates[stateId] == QColor(0, 255, 0, 110))
            m_coloredStates.remove(stateId);
        else
            m_coloredStates[stateId] = QColor(0, 255, 0, 110);
    }

    update();

    const char*   abbr = UsMap::abbrevs()[sid];
    const QString txt  = QStringLiteral("State: %1").arg(abbr);
    setToolTip(txt);
    QToolTip::showText(event->globalPos(), txt, this, QRect(), 4000);
}

void GridWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_usMap)
        return;

    uint8_t sid      = m_usMap->stateAtViewPos(event->pos(), size());
    int     newHover = (sid == UsMap::kNoState) ? -1 : int(sid);
    if (newHover == m_hoverSid)
        return; // nic nowego

    m_hoverSid = newHover;

    if (sid != UsMap::kNoState)
    {
        const char*   abbr = UsMap::abbrevs()[sid];
        const QString txt  = QStringLiteral("State: %1").arg(abbr);
        setToolTip(txt);
        QToolTip::showText(event->globalPos(), txt, this, QRect(), 3000);
    }
    else
    {
        QToolTip::hideText();
    }
}

void GridWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    m_hoverSid = -1;
    QToolTip::hideText();
}

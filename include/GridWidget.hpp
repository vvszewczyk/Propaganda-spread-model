#pragma once

#include "Simulation.hpp"
#include "Types.hpp"
#include "UsMap.hpp"

#include <QHash>
#include <QMouseEvent>
#include <QPoint>
#include <QWidget>
#include <cstdint>
#include <qelapsedtimer.h>
#include <qpainter.h>
#include <qpoint.h>

namespace app::ui
{
    class GridWidget : public QWidget
    {
            Q_OBJECT

        public:
            explicit GridWidget(QWidget* parent = nullptr);
            void setSimulation(const Simulation*) noexcept;
            void setUsMap(const UsMap*) noexcept;
            void setShowGrid(bool) noexcept;
            void clearMap() noexcept;
            void resetView() noexcept;

            [[nodiscard]] QColor getColorFor(CellData) const noexcept;

        signals:
            void cellRemoved(int x, int y);
            void zoomChanged(double zoomFactor);
            void cellInfoChanged(const QString& info);

        protected:
            void paintEvent(QPaintEvent*) override;
            void mousePressEvent(QMouseEvent*) override;
            void mouseMoveEvent(QMouseEvent* event) override;
            void mouseReleaseEvent(QMouseEvent* event) override;
            void leaveEvent(QEvent* event) override;
            void wheelEvent(QWheelEvent* event) override;

        private:
            const Simulation* m_sim{nullptr};
            const UsMap*      m_usMap{nullptr};

            bool    m_showGrid{false};
            qreal   m_zoom{1.0};
            QPointF m_pan{0.0, 0.0};
            bool    m_isPanning{false};
            QPoint  m_lastPanPos;

            QSet<int>             m_selectedStateIds;
            int                   m_selectedSingleStateId{-1};
            QHash<int, QColor>    m_coloredStates;
            QHash<QPoint, QColor> m_coloredCells;
            int                   m_hoverSid{-1};

            mutable QImage m_cellsImage;

            struct Geometry
            {
                    float cellWidth;
                    float cellHeight;
                    float mapWidth;
                    float mapHeight;
                    float originXCenter;
                    float originYCenter;
            };

            void drawGrid(QPainter& painter) const;
            bool canPaint() const noexcept;
            void drawCells(QPainter& painter, const QRectF& destRectF) const;
            void drawOutlines(QPainter& painter, const QRectF& destRectF) const;
            void drawOuterFrame(QPainter& painter) const;
            void applyBrushAt(const QPointF& pos, Qt::MouseButtons buttons);
            void updateCellInfoAt(const QPointF& position);

            [[nodiscard]] QRectF mapDestRect() const;
            void                 rebuildCellsImageIfNeeded() const;

            [[nodiscard]] QPoint  productPointFromWidgetPos(QPointF position) const;
            [[nodiscard]] uint8_t stateAtWidgetPos(QPointF position) const;
            [[nodiscard]] QString tooltipTextForState(uint8_t stateId) const;

            [[nodiscard]] Geometry computeGeometry(qreal zoom) const;

            void updatePanForZoom(const QPointF& cursorPos, qreal newZoom);
    };
} // namespace app::ui

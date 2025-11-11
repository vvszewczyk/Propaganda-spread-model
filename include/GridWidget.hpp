#pragma once

#include "Simulation.hpp"
#include "Types.hpp"
#include "UsMap.hpp"

#include <QHash>
#include <QMouseEvent>
#include <QWidget>

class GridWidget : public QWidget
{
        Q_OBJECT

    public:
        explicit GridWidget(QWidget* parent = nullptr);
        void setSimulation(const Simulation*) noexcept;
        void setUsMap(const UsMap*) noexcept;
        void setShowGrid(bool) noexcept;

        [[nodiscard]] QColor getColorFor(CellData) const noexcept;

    signals:
        void cellRemoved(int x, int y);

    protected:
        void paintEvent(QPaintEvent*) override;
        void mousePressEvent(QMouseEvent*) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;

    private:
        const Simulation* m_sim{nullptr};
        const UsMap*      m_usMap{nullptr};

        bool m_showGrid{true};

        QSet<int>          m_selectedStateIds;
        int                m_selectedSingleStateId{-1};
        QHash<int, QColor> m_coloredStates;
        int                m_hoverSid{-1};
        QImage             m_cellsImage;

        void drawCells(QPainter& painter) const;
        void drawGrid(QPainter& painter) const;
};

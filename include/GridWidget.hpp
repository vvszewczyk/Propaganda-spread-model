#pragma once

#include "Simulation.hpp"
#include "Types.hpp"
#include "UsMap.hpp"

#include <QMouseEvent>
#include <QWidget>

class GridWidget : public QWidget
{
        Q_OBJECT
        Simulation*        m_sim;
        UsMap*             m_usMap;
        bool               m_showGrid;
        bool               m_eraseMode;
        QSet<int>          m_selectedStateIds;
        int                m_selectedSingleStateId = -1;
        QHash<int, QColor> m_coloredStates;

        int    m_hoverSid = -1;
        QImage m_cellsImage;

    public:
        explicit GridWidget(QWidget* parent = nullptr);
        void   setSimulation(const Simulation*);
        void   setUsMap(const UsMap*);
        void   setShowGrid(bool);
        void   setEraseMode(bool);
        QColor getColorForCell(CellData) const;

    signals:
        void cellRemoved(int x, int y);

    protected:
        void paintEvent(QPaintEvent*) override;
        void mousePressEvent(QMouseEvent*) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
};

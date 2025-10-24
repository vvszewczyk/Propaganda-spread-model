#pragma once

#include "Simulation.hpp"
#include "Types.hpp"
#include "UsMap.hpp"

#include <QMouseEvent>
#include <QWidget>

class GridWidget : public QWidget
{
        Q_OBJECT
        Simulation* m_sim;
        UsMap*      m_usMap;
        bool        m_showGrid;
        bool        m_eraseMode;
        bool        m_debugShowSingleState = false;
        QString     m_debugStateId         = "CA";

    public:
        explicit GridWidget(QWidget* parent = nullptr);
        void   setSimulation(const Simulation*);
        void   setUsMap(const UsMap*);
        void   setShowGrid(bool);
        void   setEraseMode(bool);
        QColor getColorForCell(CellData) const;
        void   setDebugSingleState(bool on, QString id = "CA");

    signals:
        void cellRemoved(int x, int y);

    protected:
        void paintEvent(QPaintEvent*) override;
        void mousePressEvent(QMouseEvent*) override;
};

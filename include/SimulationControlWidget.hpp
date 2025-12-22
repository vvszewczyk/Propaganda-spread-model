#pragma once
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QWidget>

class SimulationControlWidget : public QWidget
{
        Q_OBJECT
    public:
        explicit SimulationControlWidget(QWidget* parent = nullptr);

        int  getSpeed() const;
        void setNeighbourhood(int index);
        void updateState(bool running);

    signals:
        void startRequested();
        void resetRequested();
        void stepRequested();
        void speedChanged(int newSpeed);
        void neighbourhoodChanged(int index);

    private:
        QPushButton* m_startButton{nullptr};
        QPushButton* m_resetButton{nullptr};
        QPushButton* m_stepButton{nullptr};
        QSlider*     m_simulationSpeedSlider{nullptr};
        QComboBox*   m_neighbourhoodCombo{nullptr};
};

#pragma once
#include "GridWidget.hpp"
#include "Simulation.hpp"
#include "UsMap.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
// #include <QtCharts>
// #include <QtWidgets/QMainWindow>
#include <memory>

class MainWindow : public QMainWindow
{
        Q_OBJECT
    public:
        MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;
        MainWindow(const MainWindow&)            = delete;
        MainWindow& operator=(const MainWindow&) = delete;
        MainWindow(MainWindow&&)                 = delete;
        MainWindow& operator=(MainWindow&&)      = delete;

    private:
        QStackedWidget*          contentStack;
        GridWidget*              gridWidget;
        std::unique_ptr<QWidget> m_statsWidget;

        std::unique_ptr<UsMap>      m_usMap;
        std::unique_ptr<Simulation> m_simulation;
        std::unique_ptr<QTimer>     m_timer;

        QLabel*      simulationLabel;
        QLabel*      iterationLabel;
        QLabel*      neighbourhoodLabel;
        QPushButton* startButton;
        QPushButton* resetButton;
        QPushButton* toggleViewButton;
        QCheckBox*   gridToggle;
        QComboBox*   neighbourhoodCombo;

        void setupUI();
        void setupLayout();
        void setupConnections();
        void updateIterationLabel(int globalStep);
        void setupStats();
        void updateStats(int globalStep);
        void clearStats();

    private slots:
        void onStartButtonClicked();
        void onResetButtonClicked();
        void onStep();
        void onNeighbourhoodChanged(int index);
        void onToggleView(bool checked);
};

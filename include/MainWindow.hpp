#pragma once
#include "GridWidget.hpp"
#include "Simulation.hpp"
#include "UsMap.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QtCharts>
#include <QtWidgets/QMainWindow>

class MainWindow : public QMainWindow
{
        Q_OBJECT
    public:
        MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;
        MainWindow(const MainWindow&)                    = delete;
        auto operator=(const MainWindow&) -> MainWindow& = delete;
        MainWindow(MainWindow&&)                         = delete;
        auto operator=(MainWindow&&) -> MainWindow&      = delete;

    private:
        QStackedWidget*             contentStack;
        GridWidget*                 gridWidget;
        QWidget*                    statsWidget;
        std::unique_ptr<UsMap>      m_usMap;
        std::unique_ptr<Simulation> sim;
        QTimer*                     timer;

        QLabel* simulationLabel;
        QLabel* iterationLabel;
        QLabel* neighbourhoodLabel;
        QLabel* iterationValueLabel;

        QPushButton* startButton;
        QPushButton* resetButton;
        QPushButton* toggleViewButton;

        QCheckBox* gridToggle;

        QComboBox* neighbourhoodCombo;

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

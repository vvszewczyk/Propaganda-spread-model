#pragma once
#include "Constants.hpp"
#include "GridWidget.hpp"
#include "PlayerControlWidget.hpp"
#include "Simulation.hpp"
#include "SimulationControlWidget.hpp"
#include "UsMap.hpp"
#include "WorldPhysicsWidget.hpp"

#include <QDebug>
#include <QElapsedTimer>
#include <QLabel>
#include <QStringLiteral>
#include <QStringView>
#include <QTimer>
#include <QtWidgets>
#include <memory>
// #include <QtCharts>
// #include <QtWidgets/QMainWindow>

namespace app::ui
{
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
            struct Model
            {
                    std::unique_ptr<UsMap>      usMap;
                    std::unique_ptr<Simulation> simulation;
                    QTimer*                     timer{};
                    QElapsedTimer               fpsTimer;
                    int                         fpsFrameCount{0};
            } model;

            struct ui
            {
                    QStackedWidget* contentStack{nullptr};
                    GridWidget*     gridWidget{nullptr};
                    QWidget*        statsWidget{nullptr};

                    SimulationControlWidget* simulationControlWidget{nullptr};
                    QTabWidget*              tabsWidget{nullptr};
                    PlayerControlWidget*     playerAWidget{nullptr};
                    PlayerControlWidget*     playerBWidget{nullptr};
                    WorldPhysicsWidget*      physics{nullptr};

                    QLabel*    scenarioPresetLabel{nullptr};
                    QComboBox* scenarioPresetCombo{nullptr};

                    QLabel*      playerSettingsLabel{nullptr};
                    QCheckBox*   gridToggle{nullptr};
                    QCheckBox*   mapToggle{nullptr};
                    QCheckBox*   paintToggle{nullptr};
                    QPushButton* toggleViewButton{nullptr};

                    QPushButton* physicsButton{nullptr};
                    QDockWidget* physicsDock{nullptr};

                    QLabel* cellInfoLabel{nullptr};
                    QLabel* iterationLabel{nullptr};
                    QLabel* fpsLabel{nullptr};
                    QLabel* zoomLabel{nullptr};
            } ui;

            enum class Page
            {
                Simulation = 0,
                Stats      = 1
            };

        private:
            void initModel();
            void createWidgets();
            void buildLayout();
            void wireAll();
            void applyDefaults();

            void wireSimulationControls();
            void wireGrid();
            void wirePlayers();
            void wireTogglesAndView();

            void doStep();
            void updateIterationLabel();
            void updateOverlayLabelsPosition();
            void countFps();
            void refreshBudgets();
            void setupPhysicsDock();

            void setupStats();
            void updateStats(int globalStep);
            void clearStats();

        private slots:
            void onStartClicked();
            void onResetClicked();
            void onStepClicked();
            void onSimulationSpeedChanged(int speed);
            void onNeighbourhoodChanged(int index);
            void onToggleView(bool checked);
    };
} // namespace app::ui

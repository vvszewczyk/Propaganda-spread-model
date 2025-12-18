#pragma once
#include "Constants.hpp"
#include "GridWidget.hpp"
#include "Simulation.hpp"
#include "UsMap.hpp"

#include <QDebug>
#include <QElapsedTimer>
#include <QLabel>
#include <QStringLiteral>
#include <QStringView>
#include <QTimer>
#include <QtWidgets>
#include <cstddef>
#include <memory>
#include <qcombobox.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <type_traits>
// #include <QtCharts>
// #include <QtWidgets/QMainWindow>

namespace app::ui
{
    template <typename T, typename Parent, typename Fn = std::nullptr_t, typename... Args>
    T* makeWidget(Parent* parent, Fn fn, Args&&... ctorArgs)
    {
        static_assert(std::is_base_of_v<QObject, T>, "T must derive from QObject / QWidget");
        auto* widget = new T(std::forward<Args>(ctorArgs)..., parent);

        if constexpr (std::is_invocable_v<Fn, T*>)
        {
            std::forward<Fn>(fn)(widget);
        }

        return widget;
    }

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
            std::unique_ptr<UsMap>      m_usMap;
            std::unique_ptr<Simulation> m_simulation;
            std::unique_ptr<QTimer>     m_timer;

            QStackedWidget* m_contentStack{nullptr};
            GridWidget*     m_gridWidget{nullptr};
            QWidget*        m_statsWidget{nullptr};

            QLabel* m_zoomLabel{nullptr};
            QLabel* m_fpsLabel{nullptr};

            QLabel*      m_simulationLabel{nullptr};
            QPushButton* m_startButton{nullptr};
            QPushButton* m_resetButton{nullptr};
            QPushButton* m_stepButton{nullptr};

            QLabel*  m_simulationSpeedLabel{nullptr};
            QSlider* m_simulationSpeedSlider{nullptr};
            QLabel*  m_slowerLabel{nullptr};
            QLabel*  m_fasterLabel{nullptr};

            QLabel*    m_neighbourhoodLabel{nullptr};
            QComboBox* m_neighbourhoodCombo{nullptr};

            QLabel*    m_scenarioPresetLabel{nullptr};
            QComboBox* m_scenarioPresetCombo{nullptr};

            QLabel* m_modelParametersLabel{nullptr};

            QLabel*         m_betaLabel{nullptr};
            QDoubleSpinBox* m_betaSpin{nullptr};
            QLabel*         m_gammaLabel{nullptr};
            QDoubleSpinBox* m_gammaSpin{nullptr};
            QLabel*         m_deltaLabel{nullptr};
            QDoubleSpinBox* m_deltaSpin{nullptr};

            // TODO: Apply button
            QLabel*       m_playerSettingsLabel{nullptr};
            QRadioButton* m_sideARadio{nullptr};
            QRadioButton* m_sideBRadio{nullptr};
            QLabel*       m_budgetLabel{nullptr};

            QCheckBox* m_gridToggle{nullptr};
            QCheckBox* m_mapToggle{nullptr};

            QLabel*      m_cellInfoLabel{nullptr};
            QPushButton* m_toggleViewButton{nullptr};

            QLabel* m_iterationLabel{nullptr};

            QElapsedTimer m_fpsTimer;
            int           m_fpsFrameCount{0};

            void buildUi();
            void buildLayout();
            void wire();
            void updateIterationLabel();
            void setupStats();
            void updateStats(int globalStep);
            void clearStats();
            void updateOverlayLabelsPosition();
            void countFps();

        private slots:
            void onStep();
            void onStartButtonClicked();
            void onResetButtonClicked();
            void onStepButtonClicked();
            void onToggleView(bool checked);
            void onNeighbourhoodChanged(int index);
            void onSimulationSpeedChanged(int speed);
    };
} // namespace app::ui

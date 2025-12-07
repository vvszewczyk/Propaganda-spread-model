#pragma once
#include "Constants.hpp"
#include "GridWidget.hpp"
#include "Simulation.hpp"
#include "UsMap.hpp"

#include <QDebug>
#include <QStringLiteral>
#include <QStringView>
#include <QTimer>
#include <QtWidgets>
#include <cstddef>
#include <memory>
#include <qelapsedtimer.h>
#include <qlabel.h>
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

            QLabel* m_simulationLabel{nullptr};
            QLabel* m_iterationLabel{nullptr};
            QLabel* m_neighbourhoodLabel{nullptr};
            QLabel* m_zoomLabel{nullptr};
            QLabel* m_fpsLabel{nullptr};
            QLabel* m_cellInfoLabel{nullptr};

            QPushButton* m_startButton{nullptr};
            QPushButton* m_resetButton{nullptr};
            QPushButton* m_toggleViewButton{nullptr};
            QCheckBox*   m_gridToggle{nullptr};
            QComboBox*   m_neighbourhoodCombo{nullptr};

            QElapsedTimer m_fpsTimer;
            int           m_fpsFrameCount{0};

            void buildUi();
            void buildLayout();
            void wire();
            void setupStats();
            void updateStats(int globalStep);
            void clearStats();
            void updateOverlayLabelsPosition();
            void countFps();

        private slots:
            void onStep();
            void onStartButtonClicked();
            void onResetButtonClicked();
            void onToggleView(bool checked);
            void onNeighbourhoodChanged(int index);
    };
} // namespace app::ui

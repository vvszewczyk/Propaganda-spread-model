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
            std::unique_ptr<UsMap>      usMap_;
            std::unique_ptr<Simulation> simulation_;
            std::unique_ptr<QTimer>     timer_;

            QStackedWidget* contentStack_{nullptr};
            GridWidget*     gridWidget_{nullptr};
            QWidget*        statsWidget_{nullptr};

            QLabel*      simulationLabel_{nullptr};
            QLabel*      iterationLabel_{nullptr};
            QLabel*      neighbourhoodLabel_{nullptr};
            QPushButton* startButton_{nullptr};
            QPushButton* resetButton_{nullptr};
            QPushButton* toggleViewButton_{nullptr};
            QCheckBox*   gridToggle_{nullptr};
            QComboBox*   neighbourhoodCombo_{nullptr};

            void buildUi();
            void buildLayout();
            void wire();
            void setupStats();
            void updateStats(int globalStep);
            void clearStats();

        private slots:
            void onStep();
            void onStartButtonClicked();
            void onResetButtonClicked();
            void onToggleView(bool checked);
            void onNeighbourhoodChanged(int index);
    };
} // namespace app::ui

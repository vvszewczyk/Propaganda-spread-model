#pragma once

#include "Simulation.hpp"

#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextStream>
#include <QWidget>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <deque>

namespace app::ui
{
    class StatsWidget : public QWidget
    {
            Q_OBJECT
        public:
            explicit StatsWidget(QWidget* parent = nullptr);

            void clear();
            void pushSample(const StepStats& s);

        private:
            void trimToMaxPoints();
            void updateAxesRanges(int lastIter);
            void updateHeader(const StepStats& s);

            void saveCsv();

        private:
            int m_maxPoints = 1200; // ile punktów trzymamy na wykresach

            QLabel*      m_header{nullptr};
            QPushButton* m_saveBtn{nullptr};
            QPushButton* m_clearBtn{nullptr};

            // Chart 1: poparcie
            QChartView*  m_popView{nullptr};
            QLineSeries* m_popA{nullptr};
            QLineSeries* m_popB{nullptr};
            QLineSeries* m_popN{nullptr};
            QValueAxis*  m_popX{nullptr};
            QValueAxis*  m_popY{nullptr};

            // Chart 2: budżety
            QChartView*  m_budgetView{nullptr};
            QLineSeries* m_budgetA{nullptr};
            QLineSeries* m_budgetB{nullptr};
            QValueAxis*  m_budgetX{nullptr};
            QValueAxis*  m_budgetY{nullptr};

            // Chart 3: sygnały
            QChartView*  m_sigView{nullptr};
            QLineSeries* m_broadcastBias{nullptr};
            QLineSeries* m_dmPressure{nullptr};
            QLineSeries* m_socialPressure{nullptr};
            QValueAxis*  m_sigX{nullptr};
            QValueAxis*  m_sigY{nullptr};

            std::deque<StepStats>  m_samples;
            std::vector<StepStats> m_allSamples;
    };
} // namespace app::ui

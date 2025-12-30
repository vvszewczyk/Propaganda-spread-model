#include "StatsWidget.hpp"

#include "Simulation.hpp"

#include <algorithm>
#include <qnamespace.h>

using namespace app::ui;

static QChartView* makeChartView(QChart* chart, QWidget* parent)
{
    auto* v = new QChartView(chart, parent);
    v->setRenderHint(QPainter::Antialiasing, true);
    return v;
}

StatsWidget::StatsWidget(QWidget* parent) : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    // Header + przyciski
    auto* topRow = new QHBoxLayout();
    m_header     = new QLabel("Stats: N/A", this);
    m_header->setStyleSheet("font-weight: bold;");

    m_saveBtn  = new QPushButton("Save CSV", this);
    m_clearBtn = new QPushButton("Clear", this);

    topRow->addWidget(m_header, 1);
    topRow->addWidget(m_saveBtn, 0);
    topRow->addWidget(m_clearBtn, 0);
    root->addLayout(topRow);

    connect(m_saveBtn, &QPushButton::clicked, this, [this] { saveCsv(); });
    connect(m_clearBtn, &QPushButton::clicked, this, [this] { clear(); });

    // --- Chart 1: poparcie ---
    auto* popChart = new QChart();
    popChart->setTitle("Support share (A / B / NONE)");
    popChart->legend()->setVisible(true);

    m_popA = new QLineSeries();
    m_popA->setName("A");
    m_popA->setColor(Qt::red);
    m_popB = new QLineSeries();
    m_popB->setName("B");
    m_popB->setColor(Qt::blue);
    m_popN = new QLineSeries();
    m_popN->setName("NONE");

    popChart->addSeries(m_popA);
    popChart->addSeries(m_popB);
    popChart->addSeries(m_popN);

    m_popX = new QValueAxis();
    m_popX->setTitleText("Iteration");
    m_popY = new QValueAxis();
    m_popY->setTitleText("Share");
    m_popY->setRange(0.0, 1.0);

    popChart->addAxis(m_popX, Qt::AlignBottom);
    popChart->addAxis(m_popY, Qt::AlignLeft);
    m_popA->attachAxis(m_popX);
    m_popA->attachAxis(m_popY);
    m_popB->attachAxis(m_popX);
    m_popB->attachAxis(m_popY);
    m_popN->attachAxis(m_popX);
    m_popN->attachAxis(m_popY);

    m_popView = makeChartView(popChart, this);

    // --- Chart 2: budżety ---
    auto* budChart = new QChart();
    budChart->setTitle("Budgets (A / B)");
    budChart->legend()->setVisible(true);

    m_budgetA = new QLineSeries();
    m_budgetA->setName("Budget A");
    m_budgetA->setColor(Qt::red);
    m_budgetB = new QLineSeries();
    m_budgetB->setName("Budget B");
    m_budgetB->setColor(Qt::blue);

    budChart->addSeries(m_budgetA);
    budChart->addSeries(m_budgetB);

    m_budgetX = new QValueAxis();
    m_budgetX->setTitleText("Iteration");
    m_budgetY = new QValueAxis();
    m_budgetY->setTitleText("Budget");

    budChart->addAxis(m_budgetX, Qt::AlignBottom);
    budChart->addAxis(m_budgetY, Qt::AlignLeft);
    m_budgetA->attachAxis(m_budgetX);
    m_budgetA->attachAxis(m_budgetY);
    m_budgetB->attachAxis(m_budgetX);
    m_budgetB->attachAxis(m_budgetY);

    m_budgetView = makeChartView(budChart, this);

    // --- Chart 3: sygnały ---
    auto* sigChart = new QChart();
    sigChart->setTitle("Signals (broadcastBias / dmPressure / socialPressure)");
    sigChart->legend()->setVisible(true);

    m_broadcastBias = new QLineSeries();
    m_broadcastBias->setName("broadcastBias");
    m_dmPressure = new QLineSeries();
    m_dmPressure->setName("dmPressure");
    m_socialPressure = new QLineSeries();
    m_socialPressure->setName("socialPressure");

    sigChart->addSeries(m_broadcastBias);
    sigChart->addSeries(m_dmPressure);
    sigChart->addSeries(m_socialPressure);

    m_sigX = new QValueAxis();
    m_sigX->setTitleText("Iteration");
    m_sigY = new QValueAxis();
    m_sigY->setTitleText("Signal");

    sigChart->addAxis(m_sigX, Qt::AlignBottom);
    sigChart->addAxis(m_sigY, Qt::AlignLeft);

    m_broadcastBias->attachAxis(m_sigX);
    m_broadcastBias->attachAxis(m_sigY);
    m_dmPressure->attachAxis(m_sigX);
    m_dmPressure->attachAxis(m_sigY);
    m_socialPressure->attachAxis(m_sigX);
    m_socialPressure->attachAxis(m_sigY);

    m_sigView = makeChartView(sigChart, this);

    // Layout wykresów: 2 + 1
    auto* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(8);

    grid->addWidget(m_popView, 0, 0, 1, 2);
    grid->addWidget(m_budgetView, 1, 0, 1, 1);
    grid->addWidget(m_sigView, 1, 1, 1, 1);

    root->addLayout(grid, 1);

    clear();
}

void StatsWidget::clear()
{
    m_samples.clear();
    m_allSamples.clear();

    m_popA->clear();
    m_popB->clear();
    m_popN->clear();
    m_budgetA->clear();
    m_budgetB->clear();
    m_broadcastBias->clear();
    m_dmPressure->clear();
    m_socialPressure->clear();

    m_popX->setRange(0, 100);
    m_budgetX->setRange(0, 100);
    m_sigX->setRange(0, 100);

    m_popY->setRange(0.0, 1.0);

    m_budgetY->setRange(0.0, 1000.0);
    m_sigY->setRange(-1.0, 1.0);

    m_header->setText("Stats: (empty)");
}

void StatsWidget::pushSample(const StepStats& s)
{
    m_allSamples.push_back(s);
    m_samples.push_back(s);

    const int it = s.iter;

    m_popA->append(it, s.shareA);
    m_popB->append(it, s.shareB);
    m_popN->append(it, s.shareN);

    m_budgetA->append(it, static_cast<double>(s.budgetA));
    m_budgetB->append(it, static_cast<double>(s.budgetB));

    m_broadcastBias->append(it, static_cast<double>(s.gSignals.broadcastBias()));
    m_dmPressure->append(it, static_cast<double>(s.gSignals.dmPressure));
    m_socialPressure->append(it, static_cast<double>(s.gSignals.socialPressure));

    trimToMaxPoints();
    updateAxesRanges(it);
    updateHeader(s);
}

void StatsWidget::trimToMaxPoints()
{
    while (static_cast<int>(m_samples.size()) > m_maxPoints)
    {
        m_samples.pop_front();
    }

    auto trimSeries = [&](QLineSeries* ser)
    {
        const int extra = ser->count() - m_maxPoints;
        if (extra > 0)
        {
            ser->removePoints(0, extra);
        }
    };

    trimSeries(m_popA);
    trimSeries(m_popB);
    trimSeries(m_popN);
    trimSeries(m_budgetA);
    trimSeries(m_budgetB);
    trimSeries(m_broadcastBias);
    trimSeries(m_dmPressure);
    trimSeries(m_socialPressure);
}

void StatsWidget::updateAxesRanges(int lastIter)
{
    const int firstIter = (m_samples.empty() ? 0 : m_samples.front().iter);
    const int minX      = std::max(firstIter, lastIter - (m_maxPoints - 1));

    m_popX->setRange(minX, lastIter);
    m_budgetX->setRange(minX, lastIter);
    m_sigX->setRange(minX, lastIter);

    // budżet Y: auto-range z próbek
    float maxBud = 1.0;
    for (const auto& s : m_samples)
    {
        maxBud = std::max({maxBud, s.budgetA, s.budgetB});
    }
    m_budgetY->setRange(0.0, static_cast<double>(maxBud) * 1.05);

    // sygnały Y: auto-range
    float maxSig = 1.0;
    for (const auto& s : m_samples)
    {
        maxSig = std::max({maxSig, std::abs(s.gSignals.broadcastBias()),
                           std::abs(s.gSignals.dmPressure), std::abs(s.gSignals.socialPressure)});
    }
    m_sigY->setRange(static_cast<double>(-maxSig) * 1.05, static_cast<double>(maxSig) * 1.05);
}

void StatsWidget::updateHeader(const StepStats& s)
{
    m_header->setText(QString("iter=%1 | active=%2 | A=%3 (%4%) | B=%5 (%6%) | NONE=%7 (%8%)")
                          .arg(s.iter)
                          .arg(s.active)
                          .arg(s.countA)
                          .arg(s.shareA * 100.0, 0, 'f', 2)
                          .arg(s.countB)
                          .arg(s.shareB * 100.0, 0, 'f', 2)
                          .arg(s.countN)
                          .arg(s.shareN * 100.0, 0, 'f', 2));
}

void StatsWidget::saveCsv()
{
    const QString path =
        QFileDialog::getSaveFileName(this, "Save stats as CSV", "stats.csv", "CSV (*.csv)");
    if (path.isEmpty())
        return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return;

    QTextStream out(&f);
    out << "iter,active,countA,countB,countN,shareA,shareB,shareN,"
           "avgHysA,avgHysB,"
           "N_to_A,N_to_B,A_to_B,B_to_A,"
           "budgetA,budgetB,plannedCostA,plannedCostB,scaleA,scaleB,spentA,spentB,"
           "ctrlA_broadcast,ctrlB_broadcast,ctrlA_dm,ctrlB_dm,ctrlA_social,ctrlB_social,"
           "effA_broadcast,effB_broadcast,effA_dm,effB_dm,effA_social,effB_social,"
           "stockA,stockB,broadcastA,broadcastB,broadcastBias,dmPressure,socialPressure,"
           "wLocal,thetaScale,margin,wDM,wBroadcast,wSocial,switchKappa,hysDecay,hysMaxTotal,"
           "broadcastDecay,broadcastStockMax,broadcastHysGain,broadcastNeutralWeight\n";

    for (const auto& s : m_allSamples)
    {
        out << s.iter << "," << s.active << "," << s.countA << "," << s.countB << "," << s.countN
            << "," << s.shareA << "," << s.shareB << "," << s.shareN << "," << s.avgHysA << ","
            << s.avgHysB << "," << s.trans.N_to_A << "," << s.trans.N_to_B << "," << s.trans.A_to_B
            << "," << s.trans.B_to_A << "," << s.budgetA << "," << s.budgetB << ","
            << s.campaign.plannedCostA << "," << s.campaign.plannedCostB << "," << s.campaign.scaleA
            << "," << s.campaign.scaleB << "," << s.campaign.spentA << "," << s.campaign.spentB
            << "," << s.campaign.ctrlA_broadcast_sum << "," << s.campaign.ctrlB_broadcast_sum << ","
            << s.campaign.ctrlA_dm_sum << "," << s.campaign.ctrlB_dm_sum << ","
            << s.campaign.ctrlA_social_sum << "," << s.campaign.ctrlB_social_sum << ","
            << s.campaign.effA_broadcast << "," << s.campaign.effB_broadcast << ","
            << s.campaign.effA_dm << "," << s.campaign.effB_dm << "," << s.campaign.effA_social
            << "," << s.campaign.effB_social << "," << s.campaign.stockA << "," << s.campaign.stockB
            << "," << s.gSignals.broadcastA << "," << s.gSignals.broadcastB << ","
            << s.gSignals.broadcastBias() << "," << s.gSignals.dmPressure << ","
            << s.gSignals.socialPressure << "," << s.paramsSnapshot.wLocal << ","
            << s.paramsSnapshot.thetaScale << "," << s.paramsSnapshot.margin << ","
            << s.paramsSnapshot.wDM << "," << s.paramsSnapshot.wBroadcast << ","
            << s.paramsSnapshot.wSocial << "," << s.paramsSnapshot.switchKappa << ","
            << s.paramsSnapshot.hysDecay << "," << s.paramsSnapshot.hysMaxTotal << ","
            << s.paramsSnapshot.broadcastDecay << "," << s.paramsSnapshot.broadcastStockMax << ","
            << s.paramsSnapshot.broadcastHysGain << "," << s.paramsSnapshot.broadcastNeutralWeight
            << "\n";
    }
}

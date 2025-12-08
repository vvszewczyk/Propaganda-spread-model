#include "MainWindow.hpp"

#include <qlabel.h>
#include <qnamespace.h>
#include <qpushbutton.h>
#include <qstringliteral.h>
#include <qtypes.h>

using namespace app::ui;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_usMap(std::make_unique<UsMap>(
          Config::Map::usSvgPath, Config::Grid::gridCols, Config::Grid::gridRows)),
      m_simulation(std::make_unique<Simulation>(Config::Grid::gridCols, Config::Grid::gridRows)),
      m_timer(std::make_unique<QTimer>(this))
{
    setFixedSize(Config::Window::width, Config::Window::height);
    buildUi();
    buildLayout();
    wire();
    m_fpsTimer.start();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi()
{
    m_contentStack = new QStackedWidget(this);
    m_gridWidget   = new GridWidget(this);
    m_statsWidget  = new QWidget(this);

    m_gridWidget->setFixedSize(Config::Grid::pixelWidth, Config::Grid::pixelHeight);
    m_gridWidget->setSimulation(m_simulation.get());

    QString error;
    if (!m_usMap->buildStateProducts(&error))
    {
        qWarning() << "Failed to build map: " << error;
    }
    m_gridWidget->setUsMap(m_usMap.get());

    m_simulationLabel    = makeWidget<QLabel>(this, nullptr, Config::UiText::simulationControl);
    m_iterationLabel     = makeWidget<QLabel>(this, nullptr, Config::UiText::iterationPrefix + "0");
    m_neighbourhoodLabel = makeWidget<QLabel>(this, nullptr, Config::UiText::neighbourhood);
    m_cellInfoLabel = makeWidget<QLabel>(this, nullptr, Config::UiText::cellInfoPrefix + "N/A");
    m_zoomLabel     = makeWidget<QLabel>(m_gridWidget, nullptr, Config::UiText::zoom + "100%");
    m_fpsLabel      = makeWidget<QLabel>(m_gridWidget, nullptr, Config::UiText::fps + "0");

    m_startButton      = makeWidget<QPushButton>(this, nullptr, Config::UiText::start);
    m_resetButton      = makeWidget<QPushButton>(this, nullptr, Config::UiText::reset);
    m_stepButton       = makeWidget<QPushButton>(this, nullptr, Config::UiText::step);
    m_toggleViewButton = makeWidget<QPushButton>(
        this, [](auto* b) { b->setCheckable(true); }, Config::UiText::showStats);

    m_gridToggle = makeWidget<QCheckBox>(
        this, [](auto* c) { c->setChecked(false); }, Config::UiText::showGrid);
    m_neighbourhoodCombo = makeWidget<QComboBox>(
        this, [](auto* c) { c->addItems({Config::UiText::vonNeumann, Config::UiText::moore}); });
    m_simulationSpeedSlider = makeWidget<QSlider>(this, /*TODO*/ nullptr, Qt::Horizontal);
}

void MainWindow::buildLayout()
{
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    // centralWidget->setStyleSheet(Config::centralWidgetColor); // lightgreen

    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(Config::Layout::mainMargin, Config::Layout::mainMargin,
                                   Config::Layout::mainMargin, Config::Layout::mainMargin);

    auto* left = new QWidget(centralWidget);
    // left->setStyleSheet(Config::leftBlockColor);
    auto* leftLayout = new QVBoxLayout(left);
    m_contentStack->addWidget(m_gridWidget);
    m_contentStack->addWidget(m_statsWidget);
    m_contentStack->setCurrentIndex(0);
    leftLayout->addWidget(m_contentStack, 1);
    leftLayout->addStretch();
    mainLayout->addWidget(left, Config::Layout::leftPanelStretch);

    auto* right = new QWidget(centralWidget);
    // right->setStyleSheet(Config::Colors::rightBlockColor);
    auto* rightLayout = new QVBoxLayout(right);
    m_simulationLabel->setStyleSheet("font-weight: bold;");
    rightLayout->addWidget(m_simulationLabel);

    // Start/pause and reset buttons
    auto* rowButtons = new QHBoxLayout();
    QSize buttonSize(Config::Layout::buttonWidth, Config::Layout::buttonHeight);
    rowButtons->addStretch();
    m_startButton->setFixedSize(buttonSize);
    rowButtons->addWidget(m_startButton);
    rowButtons->addSpacing(Config::Layout::rowButtonsSpacing); // Space between buttons
    m_resetButton->setFixedSize(buttonSize);
    rowButtons->addWidget(m_resetButton);
    rowButtons->addSpacing(Config::Layout::rowButtonsSpacing); // Space between buttons
    m_stepButton->setFixedSize(buttonSize);
    rowButtons->addWidget(m_stepButton);
    rowButtons->addStretch();
    rightLayout->addLayout(rowButtons);

    rightLayout->addWidget(m_simulationSpeedSlider);

    // Neighbourhood combobox
    rightLayout->addWidget(m_neighbourhoodLabel);
    rightLayout->addWidget(m_neighbourhoodCombo);

    // Checkboxes
    auto* rowChecks = new QHBoxLayout();
    rowChecks->addStretch();
    rowChecks->addWidget(m_gridToggle);
    rowChecks->addStretch(); // Push checkbox up
    rightLayout->addLayout(rowChecks);

    rightLayout->addStretch();

    // Cell info
    m_cellInfoLabel->setFrameShape(QFrame::StyledPanel);
    m_cellInfoLabel->setFrameShadow(QFrame::Plain);
    m_cellInfoLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_cellInfoLabel->setMinimumHeight(60);
    m_cellInfoLabel->setWordWrap(true);
    m_cellInfoLabel->setStyleSheet(
        "color: black; border: 1px solid black; border-radius: 4px; padding: 1px;");
    rightLayout->addWidget(m_cellInfoLabel);

    // Grid/stats button
    rightLayout->addWidget(m_toggleViewButton, 0, Qt::AlignCenter);

    // Iterations label
    rightLayout->addWidget(m_iterationLabel, 0, Qt::AlignRight);

    // FPS and zoom label
    m_fpsLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_fpsLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_fpsLabel->setStyleSheet("background-color: rgba(0, 0, 0, 128); color: white; padding: 1px;");

    m_zoomLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_zoomLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_zoomLabel->setStyleSheet("background-color: rgba(0, 0, 0, 128); color: white; padding: 1px;");

    updateOverlayLabelsPosition();

    // Add right panel to main layout
    mainLayout->addWidget(right, Config::Layout::rightPanelStretch);
}

void MainWindow::wire()
{
    connect(m_startButton, SIGNAL(clicked()), this, SLOT(onStartButtonClicked()));
    connect(m_resetButton, SIGNAL(clicked()), this, SLOT(onResetButtonClicked()));
    connect(m_stepButton, SIGNAL(clicked()), this, SLOT(onStepButtonClicked()));
    connect(m_gridToggle, &QCheckBox::toggled, m_gridWidget, &GridWidget::setShowGrid);
    connect(m_toggleViewButton, SIGNAL(toggled(bool)), this, SLOT(onToggleView(bool)));
    connect(m_timer.get(), SIGNAL(timeout()), this, SLOT(onStep()));
    connect(m_neighbourhoodCombo, SIGNAL(currentIndexChanged(int)), this,
            SLOT(onNeighbourhoodChanged(int)));
    connect(m_gridWidget, &GridWidget::zoomChanged, this,
            [this](double zoomFactor)
            {
                int percent = static_cast<int>(zoomFactor * 100.0 + 0.5);
                if (percent == 99 || percent == 101)
                {
                    percent = 100;
                }
                m_zoomLabel->setText(QStringLiteral("Zoom: %1%").arg(percent));
                updateOverlayLabelsPosition();
            });
    connect(m_gridWidget, &GridWidget::cellInfoChanged, this,
            [this](const QString& info)
            {
                if (info.isEmpty())
                {
                    m_cellInfoLabel->setText(QStringLiteral("Cell: N/A"));
                }
                else
                {
                    m_cellInfoLabel->setText(info);
                }
            });
}

void MainWindow::updateIterationLabel()
{
    m_iterationLabel->setText(QStringLiteral("Iteration: %1").arg(m_simulation->getIteration()));
}

void MainWindow::countFps()
{
    ++m_fpsFrameCount;
    const qint64 elapsedMs = m_fpsTimer.elapsed();
    if (elapsedMs >= 1000)
    {
        const int fpsInt = m_fpsFrameCount;
        m_fpsLabel->setText(QStringLiteral("FPS: %1").arg(fpsInt));
        updateOverlayLabelsPosition();

        m_fpsFrameCount = 0;
        m_fpsTimer.restart();
    }
}

void MainWindow::onStep()
{
    countFps();

    m_simulation->step();
    m_gridWidget->update();
    updateIterationLabel();

    if (m_contentStack->currentIndex() == 1)
    {
        // updateStats(globalStep);
    }
}

void MainWindow::onStartButtonClicked()
{
    if (m_timer->isActive())
    {
        m_timer->stop();
        m_startButton->setText(QStringLiteral("Start"));

        m_fpsLabel->setText(QStringLiteral("FPS: 0"));
        updateOverlayLabelsPosition();
    }
    else
    {
        m_fpsFrameCount = 0;
        m_fpsTimer.restart();

        m_timer->start(Config::Timing::simulationStepMs);
        m_startButton->setText(QStringLiteral("Pause"));
    }
}

void MainWindow::onResetButtonClicked()
{
    m_timer->stop();
    m_simulation->reset();
    m_fpsLabel->setText(QStringLiteral("FPS: 0"));
    updateOverlayLabelsPosition();

    m_startButton->setText(QStringLiteral("Start"));
    m_simulation->reset();

    m_gridWidget->clearMap();
    m_gridWidget->resetView();
    m_gridWidget->update();

    clearStats();
    m_iterationLabel->setText(QStringLiteral("Iteration: 0"));
}

void MainWindow::onStepButtonClicked()
{
    if (m_timer->isActive())
    {
        m_timer->stop();
        m_startButton->setText("Start");
        m_fpsLabel->setText(QStringLiteral("FPS: 0"));
        updateOverlayLabelsPosition();
    }
    else
    {
        onStep();
        m_fpsLabel->setText(QStringLiteral("FPS: 0"));
        updateOverlayLabelsPosition();
    }
}

void MainWindow::onToggleView(bool checked)
{
    m_contentStack->setCurrentIndex(checked ? 1 : 0);
    m_toggleViewButton->setText(checked ? QStringLiteral("Show simulation")
                                        : QStringLiteral("Show stats"));
}

void MainWindow::onNeighbourhoodChanged(int index)
{
    auto chosenNeighbourhoodType =
        (index == 0) ? NeighbourhoodType::MOORE : NeighbourhoodType::VON_NEUMANN;
    m_simulation->setNeighbourhoodType(chosenNeighbourhoodType);
}

void MainWindow::setupStats()
{
}

void MainWindow::updateStats(int globalStep)
{
}

void MainWindow::clearStats()
{
}

void MainWindow::updateOverlayLabelsPosition()
{
    if (not m_gridWidget)
    {
        return;
    }

    if (m_fpsLabel)
    {
        m_fpsLabel->adjustSize();
        const int x = Config::Layout::margin;
        const int y = Config::Layout::margin;
        m_fpsLabel->move(x, y);
        m_fpsLabel->raise();
    }

    if (m_zoomLabel)
    {
        m_zoomLabel->adjustSize();
        const int x = Config::Layout::margin;
        const int y = m_gridWidget->height() - m_zoomLabel->height() - Config::Layout::margin;
        m_zoomLabel->move(x, y);
        m_zoomLabel->raise();
    }
}

#include "MainWindow.hpp"

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

    m_simulationLabel    = makeWidget<QLabel>(this, nullptr, Config::UiText::simulationSettings);
    m_iterationLabel     = makeWidget<QLabel>(this, nullptr, Config::UiText::iterationPrefix + "0");
    m_neighbourhoodLabel = makeWidget<QLabel>(this, nullptr, Config::UiText::neighbourhood);

    m_startButton      = makeWidget<QPushButton>(this, nullptr, Config::UiText::start);
    m_resetButton      = makeWidget<QPushButton>(this, nullptr, Config::UiText::reset);
    m_toggleViewButton = makeWidget<QPushButton>(
        this, [](auto* b) { b->setCheckable(true); }, Config::UiText::showStats);

    m_gridToggle = makeWidget<QCheckBox>(
        this, [](auto* c) { c->setChecked(false); }, Config::UiText::showGrid);
    m_neighbourhoodCombo = makeWidget<QComboBox>(
        this, [](auto* c) { c->addItems({Config::UiText::vonNeumann, Config::UiText::moore}); });
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
    // right->setStyleSheet(Config::rightBlockColor);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->addWidget(m_simulationLabel);

    // Start/pause and reset buttons
    auto* rowButtons = new QHBoxLayout();
    rowButtons->addStretch();
    rowButtons->addWidget(m_startButton);
    rowButtons->addSpacing(Config::Layout::rowButtonsSpacing); // Space between buttons
    rowButtons->addWidget(m_resetButton);
    rowButtons->addStretch();
    rightLayout->addLayout(rowButtons);

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

    // Grid/stats button
    rightLayout->addWidget(m_toggleViewButton, 0, Qt::AlignCenter);

    // Iterations label
    rightLayout->addWidget(m_iterationLabel, 0, Qt::AlignRight);

    // Add right panel to main layout
    mainLayout->addWidget(right, Config::Layout::rightPanelStretch);
}

void MainWindow::wire()
{
    connect(m_startButton, SIGNAL(clicked()), this, SLOT(onStartButtonClicked()));
    connect(m_resetButton, SIGNAL(clicked()), this, SLOT(onResetButtonClicked()));
    connect(m_gridToggle, &QCheckBox::toggled, m_gridWidget, &GridWidget::setShowGrid);
    connect(m_toggleViewButton, SIGNAL(toggled(bool)), this, SLOT(onToggleView(bool)));
    connect(m_timer.get(), SIGNAL(timeout()), this, SLOT(onStep()));
    connect(m_neighbourhoodCombo, SIGNAL(currentIndexChanged(int)), this,
            SLOT(onNeighbourhoodChanged(int)));
}

void MainWindow::onStep()
{
    m_simulation->step();
    m_gridWidget->update();

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
    }
    else
    {
        m_timer->start(Config::Timing::simulationStepMs);
        m_startButton->setText(QStringLiteral("Pause"));
    }
}

void MainWindow::onResetButtonClicked()
{
    m_timer->stop();
    m_startButton->setText(QStringLiteral("Start"));
    m_simulation->reset();
    m_gridWidget->update();
    clearStats();
    m_iterationLabel->setText(QStringLiteral("Iteration: 0"));
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

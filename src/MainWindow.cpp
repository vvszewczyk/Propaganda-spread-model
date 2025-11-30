#include "MainWindow.hpp"

using namespace app::ui;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      usMap_(std::make_unique<UsMap>(
          Config::Map::usSvgPath, Config::Grid::gridCols, Config::Grid::gridRows)),
      simulation_(std::make_unique<Simulation>(Config::Grid::gridCols, Config::Grid::gridRows)),
      timer_(std::make_unique<QTimer>(this))
{
    setFixedSize(Config::Window::width, Config::Window::height);
    buildUi();
    buildLayout();
    wire();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi()
{
    contentStack_ = new QStackedWidget(this);
    gridWidget_   = new GridWidget(this);
    statsWidget_  = new QWidget(this);

    gridWidget_->setFixedSize(Config::Grid::pixelWidth, Config::Grid::pixelHeight);
    gridWidget_->setSimulation(simulation_.get());

    QString error;
    if (!usMap_->buildStateProducts(&error))
    {
        qWarning() << "Failed to build map: " << error;
    }
    gridWidget_->setUsMap(usMap_.get());

    simulationLabel_    = makeWidget<QLabel>(this, nullptr, Config::UiText::simulationSettings);
    iterationLabel_     = makeWidget<QLabel>(this, nullptr, Config::UiText::iterationPrefix + "0");
    neighbourhoodLabel_ = makeWidget<QLabel>(this, nullptr, Config::UiText::neighbourhood);

    startButton_      = makeWidget<QPushButton>(this, nullptr, Config::UiText::start);
    resetButton_      = makeWidget<QPushButton>(this, nullptr, Config::UiText::reset);
    toggleViewButton_ = makeWidget<QPushButton>(
        this, [](auto* b) { b->setCheckable(true); }, Config::UiText::showStats);

    gridToggle_ = makeWidget<QCheckBox>(
        this, [](auto* c) { c->setChecked(false); }, Config::UiText::showGrid);
    neighbourhoodCombo_ = makeWidget<QComboBox>(
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
    contentStack_->addWidget(gridWidget_);
    contentStack_->addWidget(statsWidget_);
    contentStack_->setCurrentIndex(0);
    leftLayout->addWidget(contentStack_, 1);
    leftLayout->addStretch();
    mainLayout->addWidget(left, Config::Layout::leftPanelStretch);

    auto* right = new QWidget(centralWidget);
    // right->setStyleSheet(Config::rightBlockColor);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->addWidget(simulationLabel_);

    // Start/pause and reset buttons
    auto* rowButtons = new QHBoxLayout();
    rowButtons->addStretch();
    rowButtons->addWidget(startButton_);
    rowButtons->addSpacing(Config::Layout::rowButtonsSpacing); // Space between buttons
    rowButtons->addWidget(resetButton_);
    rowButtons->addStretch();
    rightLayout->addLayout(rowButtons);

    // Neighbourhood combobox
    rightLayout->addWidget(neighbourhoodLabel_);
    rightLayout->addWidget(neighbourhoodCombo_);

    // Checkboxes
    auto* rowChecks = new QHBoxLayout();
    rowChecks->addStretch();
    rowChecks->addWidget(gridToggle_);
    rowChecks->addStretch(); // Push checkbox up
    rightLayout->addLayout(rowChecks);

    rightLayout->addStretch();

    // Grid/stats button
    rightLayout->addWidget(toggleViewButton_, 0, Qt::AlignCenter);

    // Iterations label
    rightLayout->addWidget(iterationLabel_, 0, Qt::AlignRight);

    // Add right panel to main layout
    mainLayout->addWidget(right, Config::Layout::rightPanelStretch);
}

void MainWindow::wire()
{
    connect(startButton_, SIGNAL(clicked()), this, SLOT(onStartButtonClicked()));
    connect(resetButton_, SIGNAL(clicked()), this, SLOT(onResetButtonClicked()));
    connect(gridToggle_, &QCheckBox::toggled, gridWidget_, &GridWidget::setShowGrid);
    connect(toggleViewButton_, SIGNAL(toggled(bool)), this, SLOT(onToggleView(bool)));
    connect(timer_.get(), SIGNAL(timeout()), this, SLOT(onStep()));
    connect(neighbourhoodCombo_, SIGNAL(currentIndexChanged(int)), this,
            SLOT(onNeighbourhoodChanged(int)));
}

void MainWindow::onStep()
{
    simulation_->step();
    gridWidget_->update();

    if (contentStack_->currentIndex() == 1)
    {
        // updateStats(globalStep);
    }
}

void MainWindow::onStartButtonClicked()
{
    if (timer_->isActive())
    {
        timer_->stop();
        startButton_->setText(QStringLiteral("Start"));
    }
    else
    {
        timer_->start(Config::Timing::simulationStepMs);
        startButton_->setText(QStringLiteral("Pause"));
    }
}

void MainWindow::onResetButtonClicked()
{
    timer_->stop();
    startButton_->setText(QStringLiteral("Start"));
    simulation_->reset();
    gridWidget_->update();
    clearStats();
    iterationLabel_->setText(QStringLiteral("Iteration: 0"));
}

void MainWindow::onToggleView(bool checked)
{
    contentStack_->setCurrentIndex(checked ? 1 : 0);
    toggleViewButton_->setText(checked ? QStringLiteral("Show simulation")
                                       : QStringLiteral("Show stats"));
}

void MainWindow::onNeighbourhoodChanged(int index)
{
    auto chosenNeighbourhoodType =
        (index == 0) ? NeighbourhoodType::MOORE : NeighbourhoodType::VON_NEUMANN;
    simulation_->setNeighbourhoodType(chosenNeighbourhoodType);
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

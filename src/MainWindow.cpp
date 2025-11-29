#include "MainWindow.hpp"

using namespace app::ui;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      usMap_(std::make_unique<UsMap>(QStringLiteral("Propaganda-spread-model/map/us_test.svg"),
                                     Config::gridCols,
                                     Config::gridRows)),
      simulation_(std::make_unique<Simulation>(Config::gridCols, Config::gridRows)),
      timer_(std::make_unique<QTimer>(this))
{
    setFixedSize(Config::windowWidth, Config::windowHeight);
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

    gridWidget_->setFixedSize(Config::gridPixelWidth, Config::gridPixelHeight);
    gridWidget_->setSimulation(simulation_.get());

    QString error;
    if (!usMap_->buildStateProducts(&error))
    {
        qWarning() << "Failed to build map: " << error;
    }
    gridWidget_->setUsMap(usMap_.get());

    simulationLabel_    = makeWidget<QLabel>(this, nullptr, QStringLiteral("Simulation settings"));
    iterationLabel_     = makeWidget<QLabel>(this, nullptr, QStringLiteral("Iteration: 0"));
    neighbourhoodLabel_ = makeWidget<QLabel>(this, nullptr, QStringLiteral("Neighbourhood"));

    startButton_      = makeWidget<QPushButton>(this, nullptr, QStringLiteral("Start"));
    resetButton_      = makeWidget<QPushButton>(this, nullptr, QStringLiteral("Reset"));
    toggleViewButton_ = makeWidget<QPushButton>(
        this, [](auto* b) { b->setCheckable(true); }, QStringLiteral("Show stats"));

    gridToggle_ = makeWidget<QCheckBox>(
        this, [](auto* c) { c->setChecked(false); }, QStringLiteral("Show grid"));
    neighbourhoodCombo_ = makeWidget<QComboBox>(
        this,
        [](auto* c) { c->addItems({QStringLiteral("Von Neumann"), QStringLiteral("Moore")}); });
}

void MainWindow::buildLayout()
{
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    // centralWidget->setStyleSheet(Config::centralWidgetColor); // lightgreen

    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto* left = new QWidget(centralWidget);
    // left->setStyleSheet(Config::leftBlockColor);
    auto* leftLayout = new QVBoxLayout(left);
    contentStack_->addWidget(gridWidget_);
    contentStack_->addWidget(statsWidget_);
    contentStack_->setCurrentIndex(0);
    leftLayout->addWidget(contentStack_, 1);
    leftLayout->addStretch();
    mainLayout->addWidget(left, 7);

    auto* right = new QWidget(centralWidget);
    // right->setStyleSheet(Config::rightBlockColor);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->addWidget(simulationLabel_);

    // Start/pause and reset buttons
    auto* rowButtons = new QHBoxLayout();
    rowButtons->addStretch();
    rowButtons->addWidget(startButton_);
    rowButtons->addSpacing(6); // Space between buttons
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
    mainLayout->addWidget(right, 1);
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
        timer_->start(100);
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

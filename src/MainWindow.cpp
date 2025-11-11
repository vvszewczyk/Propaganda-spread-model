#include "MainWindow.hpp"

#include "Constants.hpp"

#include <QDebug>
#include <QWidget>
#include <memory>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      contentStack(new QStackedWidget(centralWidget())),
      gridWidget(new GridWidget(this)),
      m_statsWidget(std::make_unique<QWidget>(this)),
      m_usMap(std::make_unique<UsMap>(
          "Propaganda-spread-model/map/us_test.svg", Config::gridCols, Config::gridRows)),
      m_simulation(std::make_unique<Simulation>(Config::gridCols, Config::gridRows)),
      m_timer(std::make_unique<QTimer>(this)),
      simulationLabel(new QLabel("Simulation settings", this)),
      iterationLabel(new QLabel("Iteration: 0", this)),
      neighbourhoodLabel(new QLabel("Neighbourhood", this)),
      startButton(new QPushButton("Start", this)),
      resetButton(new QPushButton("Reset", this)),
      toggleViewButton(new QPushButton("Show stats", this)),
      gridToggle(new QCheckBox("Show grid", this)),
      neighbourhoodCombo(new QComboBox(this))
{
    setFixedSize(Config::windowWidth, Config::windowHeight);
    setupUI();
    setupStats();
    setupLayout();
    setupConnections();
}

void MainWindow::setupUI()
{
    this->gridWidget->setFixedSize(Config::gridPixelWidth, Config::gridPixelHeight);
    this->gridWidget->setSimulation(m_simulation.get());

    m_usMap->setDebug(false, "../../../usmap_debug");
    m_usMap->setDebugColorizeStates(false);
    QString errorMessage;
    if (!m_usMap->buildProducts(&errorMessage))
    {
        qWarning() << "Failed to build US map products:" << errorMessage;
    }
    this->gridWidget->setUsMap(m_usMap.get());
    this->gridToggle->setChecked(true);

    neighbourhoodCombo->addItem("Von Neumann");
    neighbourhoodCombo->addItem("Moore");
    neighbourhoodCombo->setCurrentIndex(0);

    toggleViewButton->setCheckable(true);
    toggleViewButton->setChecked(false);
}

void MainWindow::setupLayout()
{
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    // centralWidget->setStyleSheet(Config::centralWidgetColor); // lightgreen

    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto* left = new QWidget(centralWidget);
    left->setStyleSheet(Config::leftBlockColor);
    auto* leftLayout = new QVBoxLayout(left);

    contentStack->addWidget(gridWidget);
    contentStack->addWidget(m_statsWidget.get());
    contentStack->setCurrentIndex(0);

    leftLayout->addWidget(contentStack, 1);
    leftLayout->addStretch();
    mainLayout->addWidget(left, 7);

    auto* right = new QWidget(centralWidget);
    // right->setStyleSheet(Config::rightBlockColor);
    auto* rightLayout = new QVBoxLayout(right);

    rightLayout->addWidget(simulationLabel);

    // Start/pause and reset buttons
    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(startButton);
    btnRow->addSpacing(5); // Space between buttons
    btnRow->addWidget(resetButton);
    btnRow->addStretch();
    rightLayout->addLayout(btnRow);

    // Neighbourhood combobox
    rightLayout->addWidget(neighbourhoodLabel);
    rightLayout->addWidget(neighbourhoodCombo);

    // Checkboxes
    auto* checkboxRow = new QHBoxLayout();
    checkboxRow->addStretch();
    checkboxRow->addWidget(gridToggle);
    checkboxRow->addStretch(); // Push checkbox up
    rightLayout->addLayout(checkboxRow);

    rightLayout->addStretch();

    // Grid/stats button
    rightLayout->addWidget(toggleViewButton, 0, Qt::AlignCenter);

    // Iterations label
    rightLayout->addWidget(iterationLabel, 0, Qt::AlignRight);

    // Add right panel to main layout
    mainLayout->addWidget(right, 1);
}

void MainWindow::setupStats()
{
}

void MainWindow::setupConnections()
{
    connect(startButton, SIGNAL(clicked()), this,
            SLOT(onStartButtonClicked())); // Start button event
    connect(resetButton, SIGNAL(clicked()), this,
            SLOT(onResetButtonClicked())); // Reset button event
    connect(gridToggle, &QCheckBox::toggled, gridWidget, &GridWidget::setShowGrid);
    connect(toggleViewButton, SIGNAL(toggled(bool)), this,
            SLOT(onToggleView(bool)));                               // Toggle view button
    connect(m_timer.get(), SIGNAL(timeout()), this, SLOT(onStep())); // Timer timeout
    connect(neighbourhoodCombo, SIGNAL(currentIndexChanged(int)), this,
            SLOT(onNeighbourhoodChanged(int))); // Neighbourhood combobox
}

// Widget methods

void MainWindow::onStartButtonClicked()
{
    if (m_timer->isActive())
    {
        m_timer->stop();
        startButton->setText("Start");
    }
    else
    {
        m_timer->start(100);
        startButton->setText("Pause");
    }
}

void MainWindow::onResetButtonClicked()
{
    m_timer->stop();
    startButton->setText("Start");
    m_simulation->reset();
    gridWidget->update();
    clearStats();
    updateIterationLabel(0);
}

void MainWindow::onStep()
{
    // CA
    m_simulation->step();
    gridWidget->update();

    if (contentStack->currentIndex() == 1)
    {
        // updateStats(globalStep);
    }
}

void MainWindow::updateIterationLabel(int globalStep)
{
    iterationLabel->setText(QString("Iteration: %1").arg(globalStep));
}

void MainWindow::onNeighbourhoodChanged(int index)
{
    auto chosenNeighbourhoodType = NeighbourhoodType::VON_NEUMANN;

    if (index == 0)
    {
        chosenNeighbourhoodType = NeighbourhoodType::VON_NEUMANN;
    }
    else if (index == 1)
    {
        chosenNeighbourhoodType = NeighbourhoodType::MOORE;
    }
    m_simulation->setNeighbourhoodType(chosenNeighbourhoodType);
}

void MainWindow::onToggleView(bool checked)
{
    if (checked)
    {
        contentStack->setCurrentIndex(1);
        toggleViewButton->setText(QStringLiteral("Show simulation"));
        // updateStats(globalStep);
    }
    else
    {
        contentStack->setCurrentIndex(0);
        toggleViewButton->setText(QStringLiteral("Show stats"));
    }
}

void MainWindow::updateStats(int globalStep)
{
}

void MainWindow::clearStats()
{
}

MainWindow::~MainWindow() = default;

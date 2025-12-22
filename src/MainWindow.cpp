#include "MainWindow.hpp"

#include "GridWidget.hpp"
#include "PlayerControlWidget.hpp"
#include "SimulationControlWidget.hpp"
#include "WorldPhysicsWidget.hpp"

#include <QBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QStringLiteral>
#include <UiUtils.hpp>

using namespace app::ui;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setFixedSize(Config::Window::width, Config::Window::height);

    initModel();
    createWidgets();
    buildLayout();
    setupPhysicsDock();
    wireAll();
    applyDefaults();

    model.fpsTimer.start();
}

MainWindow::~MainWindow() = default;

void MainWindow::initModel()
{
    model.usMap = std::make_unique<UsMap>(Config::Map::usSvgPath, Config::Grid::gridCols,
                                          Config::Grid::gridRows);

    model.simulation = std::make_unique<Simulation>(Config::Grid::gridCols, Config::Grid::gridRows);

    model.timer = new QTimer(this);
}

void MainWindow::createWidgets()
{
    ui.contentStack = new QStackedWidget(this);

    ui.gridWidget = new GridWidget(this);
    ui.gridWidget->setSimulation(model.simulation.get());
    ui.gridWidget->setFixedSize(Config::Grid::pixelWidth, Config::Grid::pixelHeight);

    ui.statsWidget = new QWidget(this);

    ui.contentStack->addWidget(ui.gridWidget);
    ui.contentStack->addWidget(ui.statsWidget);
    ui.contentStack->setCurrentIndex(static_cast<int>(Page::Simulation));

    ui.simulationControlWidget = new SimulationControlWidget(this);

    ui.physics = new WorldPhysicsWidget(this);

    ui.tabsWidget    = new QTabWidget(this);
    ui.playerAWidget = new PlayerControlWidget("Side A", this);
    ui.playerBWidget = new PlayerControlWidget("Side B", this);
    ui.tabsWidget->addTab(ui.playerAWidget, "Player A");
    ui.tabsWidget->addTab(ui.playerBWidget, "Player B");

    ui.scenarioPresetLabel = makeWidget<QLabel>(this, nullptr, Config::UiText::scenarioPreset);
    ui.scenarioPresetCombo = makeWidget<QComboBox>(
        this, [](auto* comboBox) { comboBox->addItems({Config::UiText::scenario1}); });

    ui.playerSettingsLabel = makeWidget<QLabel>(this, nullptr, Config::UiText::playerSettings);
    ui.gridToggle          = makeWidget<QCheckBox>(
        this, [](auto* c) { c->setChecked(false); }, Config::UiText::showGrid);
    ui.mapToggle =
        makeWidget<QCheckBox>(this, [](auto* c) { c->setChecked(false); }, Config::UiText::useMap);
    ui.paintToggle = makeWidget<QCheckBox>(
        this, [](auto* c) { c->setChecked(false); }, Config::UiText::paintMode);

    ui.toggleViewButton = makeWidget<QPushButton>(
        this, [](auto* b) { b->setCheckable(true); }, Config::UiText::showStats);
    ui.physicsButton = makeWidget<QPushButton>(
        this,
        [](QPushButton* b)
        {
            b->setCheckable(true);
            b->setChecked(false);
        },
        Config::UiText::physics);

    ui.cellInfoLabel  = makeWidget<QLabel>(this, nullptr, Config::UiText::cellInfoPrefix + "N/A");
    ui.iterationLabel = makeWidget<QLabel>(this, nullptr, Config::UiText::iterationPrefix + "0");

    ui.zoomLabel = makeWidget<QLabel>(ui.gridWidget, nullptr, Config::UiText::zoom + "100%");
    ui.fpsLabel  = makeWidget<QLabel>(ui.gridWidget, nullptr, Config::UiText::fps + "0");

    QString error;
    if (!model.usMap->buildStateProducts(&error))
    {
        qWarning() << "Failed to build map: " << error;
    }
    ui.gridWidget->setUsMap(model.usMap.get());
}

void MainWindow::setupPhysicsDock()
{
    ui.physicsDock = new QDockWidget(tr("Settings"), this);

    ui.physicsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                                    Qt::BottomDockWidgetArea);

    ui.physicsDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    auto* scroll = new QScrollArea(ui.physicsDock);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    ui.physics->setParent(scroll);
    scroll->setWidget(ui.physics);
    ui.physicsDock->setWidget(scroll);

    ui.physicsDock->hide();
    ui.physicsDock->setFloating(true);
    ui.physicsDock->resize(420, 600);
    ui.physicsDock->move(this->geometry().center() - QPoint(210, 300));

    connect(ui.physicsButton, &QPushButton::toggled, this,
            [this](bool on)
            {
                ui.physicsDock->setVisible(on);

                if (on)
                {
                    ui.physicsDock->raise();
                }
            });

    connect(ui.physicsDock, &QDockWidget::visibilityChanged, this,
            [this](bool visible)
            {
                if (ui.physicsButton->isChecked() != visible)
                {
                    ui.physicsButton->blockSignals(true);
                    ui.physicsButton->setChecked(visible);
                    ui.physicsButton->blockSignals(false);
                }
            });
}

void MainWindow::buildLayout()
{
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(Config::Layout::mainMargin, Config::Layout::mainMargin,
                                   Config::Layout::mainMargin, Config::Layout::mainMargin);

    auto* left       = new QWidget(centralWidget);
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->addWidget(ui.contentStack, 1);
    leftLayout->addStretch();

    mainLayout->addWidget(left, Config::Layout::leftPanelStretch);

    auto* right       = new QWidget(centralWidget);
    auto* rightLayout = new QVBoxLayout(right);
    // right->setObjectName("RightPanel");
    // right->setStyleSheet(
    //     R"(QWidget#RightPanel {border: 1px solid black; background: transparent;})");

    rightLayout->addWidget(ui.simulationControlWidget);

    // Scenario preset comboBox
    rightLayout->addWidget(ui.scenarioPresetLabel);
    rightLayout->addWidget(ui.scenarioPresetCombo);

    // Checkboxes
    auto* checksLayout = new QHBoxLayout();
    checksLayout->addWidget(ui.gridToggle);
    checksLayout->addWidget(ui.mapToggle);
    rightLayout->addLayout(checksLayout);

    // Player settings
    ui.playerSettingsLabel->setStyleSheet("font-weight: bold; font-size: 13px;");
    rightLayout->addWidget(ui.playerSettingsLabel);
    rightLayout->addWidget(ui.paintToggle);

    // Player tabs
    rightLayout->addWidget(ui.tabsWidget, 1);

    rightLayout->addStretch();

    // Cell info
    ui.cellInfoLabel->setFrameShape(QFrame::StyledPanel);
    ui.cellInfoLabel->setFrameShadow(QFrame::Plain);
    ui.cellInfoLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ui.cellInfoLabel->setMinimumHeight(60);
    ui.cellInfoLabel->setWordWrap(true);
    ui.cellInfoLabel->setStyleSheet(
        "color: black; border: 1px solid black; border-radius: 4px; padding: 1px;");
    rightLayout->addWidget(ui.cellInfoLabel);

    // Toggle view/physics button
    auto* buttonsLayout = new QHBoxLayout();
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(ui.toggleViewButton);
    buttonsLayout->addWidget(ui.physicsButton);
    buttonsLayout->addStretch();
    rightLayout->addLayout(buttonsLayout);

    // Iteration label
    rightLayout->addWidget(ui.iterationLabel, 0, Qt::AlignRight);

    // Overlay styling
    ui.fpsLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui.fpsLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui.fpsLabel->setStyleSheet("background-color: rgba(0, 0, 0, 128); color: white; padding: 1px;");

    ui.zoomLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui.zoomLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui.zoomLabel->setStyleSheet(
        "background-color: rgba(0, 0, 0, 128); color: white; padding: 1px;");

    updateOverlayLabelsPosition();

    mainLayout->addWidget(right, Config::Layout::rightPanelStretch);
}

void MainWindow::wireAll()
{
    wireSimulationControls();
    wirePlayers();
    wireGrid();
    wireTogglesAndView();

    connect(model.timer, &QTimer::timeout, this, [this] { doStep(); });
}

void MainWindow::wireSimulationControls()
{
    connect(ui.simulationControlWidget, &SimulationControlWidget::startRequested, this,
            &MainWindow::onStartClicked);
    connect(ui.simulationControlWidget, &SimulationControlWidget::resetRequested, this,
            &MainWindow::onResetClicked);
    connect(ui.simulationControlWidget, &SimulationControlWidget::stepRequested, this,
            &MainWindow::onStepClicked);
    connect(ui.simulationControlWidget, &SimulationControlWidget::speedChanged, this,
            &MainWindow::onSimulationSpeedChanged);
    connect(ui.simulationControlWidget, &SimulationControlWidget::neighbourhoodChanged, this,
            &MainWindow::onNeighbourhoodChanged);
    connect(ui.physics, &WorldPhysicsWidget::parametersChanged, this,
            [this]() { model.simulation->setParameters(ui.physics->getParameters()); });
}

void MainWindow::wirePlayers()
{
    auto syncPlayers = [this]()
    {
        auto pA = model.simulation->getPlayerA();
        auto pB = model.simulation->getPlayerB();

        pA.controls = ui.playerAWidget->getControls();
        pB.controls = ui.playerBWidget->getControls();

        model.simulation->setPlayers(pA, pB);

        refreshBudgets();
    };

    connect(ui.playerAWidget, &PlayerControlWidget::controlsChanged, this, syncPlayers);
    connect(ui.playerBWidget, &PlayerControlWidget::controlsChanged, this, syncPlayers);
}

void MainWindow::wireGrid()
{
    connect(ui.gridWidget, &GridWidget::zoomChanged, this,
            [this](double zoomFactor)
            {
                int percent = static_cast<int>(zoomFactor * 100.0 + 0.5);
                if (percent == 99 || percent == 101)
                {
                    percent = 100;
                }
                ui.zoomLabel->setText(QStringLiteral("Zoom: %1%").arg(percent));
                updateOverlayLabelsPosition();
            });

    connect(ui.gridWidget, &GridWidget::cellInfoChanged, this, [this](const QString& info)
            { ui.cellInfoLabel->setText(info.isEmpty() ? QStringLiteral("Cell N/A") : info); });

    connect(ui.gridWidget, &GridWidget::paintCellRequested, this,
            [this](int x, int y, Side side)
            {
                model.simulation->cellAt(x, y).side = side;
                ui.gridWidget->update();
            });
}

void MainWindow::wireTogglesAndView()
{
    connect(ui.gridToggle, &QCheckBox::toggled, ui.gridWidget, &GridWidget::setShowGrid);
    connect(ui.mapToggle, &QCheckBox::toggled, ui.gridWidget, &GridWidget::setMapMode);
    connect(ui.paintToggle, &QCheckBox::toggled, ui.gridWidget, &GridWidget::setPaintMode);

    connect(ui.toggleViewButton, &QPushButton::toggled, this, &MainWindow::onToggleView);
}

void MainWindow::applyDefaults()
{
    onSimulationSpeedChanged(ui.simulationControlWidget->getSpeed());
    ui.simulationControlWidget->setNeighbourhood(0);

    BaseParameters defaults;
    defaults.wLocal = 1.0f;
    defaults.wDM    = 1.0f;

    ui.physics->setParameters(defaults);
    model.simulation->setParameters(defaults);

    model.simulation->setAllThreshold(0.3);

    refreshBudgets();
    updateIterationLabel();
}

void MainWindow::refreshBudgets()
{
    const auto pA = model.simulation->getPlayerA();
    const auto pB = model.simulation->getPlayerB();

    ui.playerAWidget->updateBudgetDisplay(pA.budget, pA.calculatePlannedCost());
    ui.playerBWidget->updateBudgetDisplay(pB.budget, pB.calculatePlannedCost());
}

void MainWindow::updateIterationLabel()
{
    ui.iterationLabel->setText(
        QStringLiteral("Iteration: %1").arg(model.simulation->getIteration()));
}

void MainWindow::countFps()
{
    ++model.fpsFrameCount;
    const qint64 elapsedMs = model.fpsTimer.elapsed();
    if (elapsedMs >= 1000)
    {
        ui.fpsLabel->setText(QStringLiteral("FPS: %1").arg(model.fpsFrameCount));
        updateOverlayLabelsPosition();

        model.fpsFrameCount = 0;
        model.fpsTimer.restart();
    }
}

void MainWindow::updateOverlayLabelsPosition()
{
    if (not ui.gridWidget)
    {
        return;
    }

    if (ui.fpsLabel)
    {
        ui.fpsLabel->adjustSize();
        ui.fpsLabel->move(Config::Layout::margin, Config::Layout::margin);
        ui.fpsLabel->raise();
    }

    if (ui.zoomLabel)
    {
        ui.zoomLabel->adjustSize();
        const int x = Config::Layout::margin;
        const int y = ui.gridWidget->height() - ui.zoomLabel->height() - Config::Layout::margin;
        ui.zoomLabel->move(x, y);
        ui.zoomLabel->raise();
    }
}

void MainWindow::doStep()
{
    countFps();

    model.simulation->step();
    ui.gridWidget->update();

    updateIterationLabel();
    refreshBudgets();

    if (ui.contentStack->currentIndex() == static_cast<int>(Page::Stats))
    {
        // updateStats(globalStep);
    }
}

void MainWindow::onStartClicked()
{
    if (model.timer->isActive())
    {
        model.timer->stop();
        ui.simulationControlWidget->updateState(false);
        return;
    }
    else
    {
        model.fpsFrameCount = 0;
        model.fpsTimer.restart();

        model.timer->start();
        ui.simulationControlWidget->updateState(true);
    }
}

void MainWindow::onResetClicked()
{
    model.timer->stop();
    model.fpsFrameCount = 0;

    model.simulation->reset();

    ui.gridWidget->clearMap();
    ui.gridWidget->resetView();
    ui.gridWidget->update();

    updateOverlayLabelsPosition();

    clearStats();

    ui.fpsLabel->setText(QStringLiteral("FPS: 0"));
    ui.iterationLabel->setText(QStringLiteral("Iteration: 0"));
    ui.simulationControlWidget->updateState(false);

    refreshBudgets();
}

void MainWindow::onStepClicked()
{
    if (model.timer->isActive())
    {
        model.timer->stop();
        ui.simulationControlWidget->updateState(false);
    }

    doStep();
}

void MainWindow::onToggleView(bool checked)
{
    ui.contentStack->setCurrentIndex(checked ? static_cast<int>(Page::Stats)
                                             : static_cast<int>(Page::Simulation));

    ui.toggleViewButton->setText(checked ? QStringLiteral("Show simulation")
                                         : QStringLiteral("Show stats"));
}

void MainWindow::onNeighbourhoodChanged(int index)
{
    auto chosenNeighbourhoodType =
        (index == 0) ? NeighbourhoodType::VON_NEUMANN : NeighbourhoodType::MOORE;
    model.simulation->setNeighbourhoodType(chosenNeighbourhoodType);
}

void MainWindow::onSimulationSpeedChanged(int speed)
{
    const int slowInterval = 1000;
    const int fastInterval = 16;

    float t           = static_cast<float>(speed) / 100.0f;
    int   newInterval = static_cast<int>(slowInterval + t * (fastInterval - slowInterval));

    model.timer->setInterval(newInterval);
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

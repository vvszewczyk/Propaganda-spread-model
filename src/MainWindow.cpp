#include "MainWindow.hpp"

#include <QBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QStringLiteral>
#include <qcombobox.h>
#include <qlabel.h>
#include <qnamespace.h>
#include <qradiobutton.h>
#include <qspinbox.h>

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

    BaseParameters p{};
    p.wLocal      = 1.0f;
    p.wDM         = 1.0f;
    p.wBroadcast  = 0.0f;
    p.wSocial     = 0.0f;
    p.thetaScale  = 1.0f;
    p.margin      = 0.05f;
    p.switchKappa = 2.0f;
    p.hysGrow     = 0.02f;
    p.hysDecay    = 0.01f;

    m_simulation->setParameters(p);
    m_simulation->setAllThreshold(0.3);

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

    m_simulationLabel = makeWidget<QLabel>(this, nullptr, Config::UiText::simulationControl);
    m_startButton     = makeWidget<QPushButton>(this, nullptr, Config::UiText::start);
    m_resetButton     = makeWidget<QPushButton>(this, nullptr, Config::UiText::reset);
    m_stepButton      = makeWidget<QPushButton>(this, nullptr, Config::UiText::step);

    m_simulationSpeedLabel  = makeWidget<QLabel>(this, nullptr, Config::UiText::simulationSpeed);
    m_simulationSpeedSlider = makeWidget<QSlider>(
        this,
        [](QSlider* slider)
        {
            slider->setRange(0, 100);
            slider->setValue(Config::Timing::simulationSpeed);
        },
        Qt::Horizontal);
    m_slowerLabel = makeWidget<QLabel>(this, nullptr, Config::UiText::slower);
    m_fasterLabel = makeWidget<QLabel>(this, nullptr, Config::UiText::faster);

    m_neighbourhoodLabel = makeWidget<QLabel>(this, nullptr, Config::UiText::neighbourhood);
    m_neighbourhoodCombo = makeWidget<QComboBox>(
        this, [](auto* comboBox)
        { comboBox->addItems({Config::UiText::vonNeumann, Config::UiText::moore}); });

    m_scenarioPresetLabel = makeWidget<QLabel>(this, nullptr, Config::UiText::scenarioPreset);
    m_scenarioPresetCombo = makeWidget<QComboBox>(
        this, [](auto* comboBox) { comboBox->addItems({Config::UiText::scenario1}); });

    m_modelParametersLabel = makeWidget<QLabel>(this, nullptr, Config::UiText::modelParameters);
    m_betaLabel            = makeWidget<QLabel>(this, nullptr, Config::UiText::beta);
    m_betaSpin             = makeWidget<QDoubleSpinBox>(this,
                                                        [](QDoubleSpinBox* spin)
                                                        {
                                                spin->setRange(Config::UiValues::minSpin,
                                                                           Config::UiValues::maxSpin);
                                                spin->setSingleStep(Config::UiValues::stepSpin);
                                                spin->setValue(Config::UiValues::defaultSpin);
                                            });
    m_gammaLabel           = makeWidget<QLabel>(this, nullptr, Config::UiText::gamma);
    m_gammaSpin            = makeWidget<QDoubleSpinBox>(this,
                                                        [](QDoubleSpinBox* spin)
                                                        {
                                                 spin->setRange(Config::UiValues::minSpin,
                                                                           Config::UiValues::maxSpin);
                                                 spin->setSingleStep(Config::UiValues::stepSpin);
                                                 spin->setValue(Config::UiValues::defaultSpin);
                                             });
    m_deltaLabel           = makeWidget<QLabel>(this, nullptr, Config::UiText::delta);
    m_deltaSpin            = makeWidget<QDoubleSpinBox>(this,
                                                        [](QDoubleSpinBox* spin)
                                                        {
                                                 spin->setRange(Config::UiValues::minSpin,
                                                                           Config::UiValues::maxSpin);
                                                 spin->setSingleStep(Config::UiValues::stepSpin);
                                                 spin->setValue(Config::UiValues::defaultSpin);
                                             });

    m_gridToggle = makeWidget<QCheckBox>(
        this, [](auto* checkbox) { checkbox->setChecked(false); }, Config::UiText::showGrid);
    m_mapToggle = makeWidget<QCheckBox>(
        this, [](auto* checkbox) { checkbox->setChecked(false); }, Config::UiText::useMap);

    m_playerSettingsLabel = makeWidget<QLabel>(this, nullptr, Config::UiText::playerSettings);
    m_paintToggle         = makeWidget<QCheckBox>(
        this, [](auto* checkbox) { checkbox->setChecked(false); }, Config::UiText::paintMode);
    m_sideARadio = makeWidget<QRadioButton>(
        this, [](auto* radio) { radio->setChecked(true); }, Config::UiText::playerA);
    m_sideBRadio  = makeWidget<QRadioButton>(this, nullptr, Config::UiText::playerB);
    m_budgetLabel = makeWidget<QLabel>(this, nullptr, Config::UiText::budget + "N/A");

    m_cellInfoLabel = makeWidget<QLabel>(this, nullptr, Config::UiText::cellInfoPrefix + "N/A");
    m_zoomLabel     = makeWidget<QLabel>(m_gridWidget, nullptr, Config::UiText::zoom + "100%");
    m_fpsLabel      = makeWidget<QLabel>(m_gridWidget, nullptr, Config::UiText::fps + "0");

    m_toggleViewButton = makeWidget<QPushButton>(
        this, [](auto* button) { button->setCheckable(true); }, Config::UiText::showStats);

    m_iterationLabel = makeWidget<QLabel>(this, nullptr, Config::UiText::iterationPrefix + "0");
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
    m_simulationLabel->setStyleSheet("font-weight: bold; font-size: 13px;");
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

    // Simulation speed slider
    rightLayout->addWidget(m_simulationSpeedLabel);
    auto* speedControlLayout = new QHBoxLayout();
    speedControlLayout->setContentsMargins(0, 0, 0, 0);
    m_slowerLabel->setAlignment(Qt::AlignCenter);
    m_slowerLabel->setStyleSheet("font-size: 11pt;");
    m_fasterLabel->setAlignment(Qt::AlignCenter);
    m_fasterLabel->setStyleSheet("font-size: 11pt;");
    speedControlLayout->addWidget(m_slowerLabel);
    speedControlLayout->addWidget(m_simulationSpeedSlider);
    speedControlLayout->addWidget(m_fasterLabel);
    rightLayout->addLayout(speedControlLayout);

    // Neighbourhood comboBox
    rightLayout->addWidget(m_neighbourhoodLabel);
    rightLayout->addWidget(m_neighbourhoodCombo);

    // Scenario preset comboBox
    rightLayout->addWidget(m_scenarioPresetLabel);
    rightLayout->addWidget(m_scenarioPresetCombo);

    // Checkboxes
    auto* rowChecks = new QHBoxLayout();
    rowChecks->addWidget(m_gridToggle);
    rowChecks->addWidget(m_mapToggle);
    rightLayout->addLayout(rowChecks);

    m_modelParametersLabel->setStyleSheet("font-weight: bold; font-size: 13px;");
    rightLayout->addWidget(m_modelParametersLabel);
    auto* rowSpinSection = new QHBoxLayout();
    rowSpinSection->addStretch();
    QSize spinBoxSize(Config::Layout::buttonWidth, Config::Layout::buttonHeight);
    auto* betaGroup = new QVBoxLayout();
    betaGroup->setSpacing(0);
    m_betaSpin->setFixedSize(spinBoxSize);
    m_betaLabel->setAlignment(Qt::AlignCenter);
    betaGroup->addWidget(m_betaSpin);
    betaGroup->addWidget(m_betaLabel);
    rowSpinSection->addLayout(betaGroup);
    rowSpinSection->addSpacing(Config::Layout::rowButtonsSpacing);
    auto* gammaGroup = new QVBoxLayout();
    gammaGroup->setSpacing(0);
    m_gammaSpin->setFixedSize(spinBoxSize);
    m_gammaLabel->setAlignment(Qt::AlignCenter);
    gammaGroup->addWidget(m_gammaSpin);
    gammaGroup->addWidget(m_gammaLabel);
    rowSpinSection->addLayout(gammaGroup);
    rowSpinSection->addSpacing(Config::Layout::rowButtonsSpacing);
    auto* deltaGroup = new QVBoxLayout();
    deltaGroup->setSpacing(0);
    m_deltaSpin->setFixedSize(spinBoxSize);
    m_deltaLabel->setAlignment(Qt::AlignCenter);
    deltaGroup->addWidget(m_deltaSpin);
    deltaGroup->addWidget(m_deltaLabel);
    rowSpinSection->addLayout(deltaGroup);
    rowSpinSection->addStretch();
    rightLayout->addLayout(rowSpinSection);

    // Player settings
    m_playerSettingsLabel->setStyleSheet("font-weight: bold; font-size: 13px;");
    rightLayout->addWidget(m_playerSettingsLabel);
    rightLayout->addWidget(m_paintToggle);
    auto* playerRadioLayout = new QHBoxLayout();
    playerRadioLayout->addStretch();
    playerRadioLayout->addWidget(m_sideARadio);
    playerRadioLayout->addSpacing(Config::Layout::rowButtonsSpacing * 2);
    playerRadioLayout->addWidget(m_sideBRadio);
    playerRadioLayout->addStretch();
    rightLayout->addLayout(playerRadioLayout);
    rightLayout->addWidget(m_budgetLabel);

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
    connect(m_gridWidget, &GridWidget::paintCellRequested, this,
            [this](int x, int y, Side side)
            {
                m_simulation->cellAt(x, y).side = side;
                m_gridWidget->update();
            });
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
    connect(m_resetButton, &QPushButton::clicked, this, &MainWindow::onResetButtonClicked);
    connect(m_stepButton, &QPushButton::clicked, this, &MainWindow::onStepButtonClicked);
    connect(m_simulationSpeedSlider, &QSlider::valueChanged, this,
            &MainWindow::onSimulationSpeedChanged);
    connect(m_gridToggle, &QCheckBox::toggled, m_gridWidget, &GridWidget::setShowGrid);
    connect(m_mapToggle, &QCheckBox::toggled, m_gridWidget, &GridWidget::setMapMode);
    connect(m_paintToggle, &QCheckBox::toggled, m_gridWidget, &GridWidget::setPaintMode);
    connect(m_toggleViewButton, &QPushButton::toggled, this, &MainWindow::onToggleView);
    connect(m_timer.get(), &QTimer::timeout, this, [this] { onStep(); });
    connect(m_neighbourhoodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::onNeighbourhoodChanged);
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

        onSimulationSpeedChanged(m_simulationSpeedSlider->value());
        m_timer->start();
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
    m_simulationSpeedSlider->setValue(Config::Timing::simulationSpeed);
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
        (index == 0) ? NeighbourhoodType::VON_NEUMANN : NeighbourhoodType::MOORE;
    m_simulation->setNeighbourhoodType(chosenNeighbourhoodType);
}

void MainWindow::onSimulationSpeedChanged(int speed)
{
    const int slowInterval = 1000;
    const int fastInterval = 16;

    float t           = static_cast<float>(speed) / 100.0f;
    int   newInterval = static_cast<int>(slowInterval + t * (fastInterval - slowInterval));

    m_timer->setInterval(newInterval);
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

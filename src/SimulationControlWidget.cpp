#include "SimulationControlWidget.hpp"

#include "Constants.hpp"
#include "UiUtils.hpp"

#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <qobject.h>

using namespace app::ui;

SimulationControlWidget::SimulationControlWidget(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    auto* title = makeWidget<QLabel>(
        this, [](QLabel* l) { l->setStyleSheet("font-weight: bold; font-size: 13px;"); },
        Config::UiText::simulationControl);
    layout->addWidget(title);

    auto* btnLayout = new QHBoxLayout();
    m_startButton   = makeWidget<QPushButton>(this, nullptr, Config::UiText::start);
    m_resetButton   = makeWidget<QPushButton>(this, nullptr, Config::UiText::reset);
    m_stepButton    = makeWidget<QPushButton>(this, nullptr, Config::UiText::step);

    const QSize btnSize(Config::Layout::buttonWidth, Config::Layout::buttonHeight);
    m_startButton->setFixedSize(btnSize);
    m_resetButton->setFixedSize(btnSize);
    m_stepButton->setFixedSize(btnSize);

    btnLayout->addWidget(m_startButton);
    btnLayout->addWidget(m_resetButton);
    btnLayout->addWidget(m_stepButton);
    layout->addLayout(btnLayout);

    layout->addWidget(new QLabel(Config::UiText::simulationSpeed));
    auto* speedLayout = new QHBoxLayout();

    auto* lblSlow = new QLabel(Config::UiText::slower);
    auto* lblFast = new QLabel(Config::UiText::faster);
    lblSlow->setStyleSheet("font-size: 10px; color: gray;");
    lblFast->setStyleSheet("font-size: 10px; color: gray;");

    m_simulationSpeedSlider = makeWidget<QSlider>(
        this,
        [](QSlider* s)
        {
            s->setRange(0, 100);
            s->setValue(Config::Timing::simulationSpeed);
        },
        Qt::Horizontal);

    speedLayout->addWidget(lblSlow);
    speedLayout->addWidget(m_simulationSpeedSlider);
    speedLayout->addWidget(lblFast);
    layout->addLayout(speedLayout);

    layout->addWidget(new QLabel(Config::UiText::neighbourhood));
    m_neighbourhoodCombo = makeWidget<QComboBox>(
        this,
        [](QComboBox* c) { c->addItems({Config::UiText::vonNeumann, Config::UiText::moore}); });
    layout->addWidget(m_neighbourhoodCombo);

    connect(m_startButton, &QPushButton::clicked, this, &SimulationControlWidget::startRequested);
    connect(m_resetButton, &QPushButton::clicked, this, &SimulationControlWidget::resetRequested);
    connect(m_stepButton, &QPushButton::clicked, this, &SimulationControlWidget::stepRequested);

    connect(m_simulationSpeedSlider, &QSlider::valueChanged, this,
            &SimulationControlWidget::speedChanged);

    connect(m_neighbourhoodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &SimulationControlWidget::neighbourhoodChanged);
}

int SimulationControlWidget::getSpeed() const
{
    return m_simulationSpeedSlider->value();
}

void SimulationControlWidget::setNeighbourhood(int index)
{
    if (index < 0 || index >= m_neighbourhoodCombo->count())
    {
        return;
    }

    m_neighbourhoodCombo->setCurrentIndex(index);
}

void SimulationControlWidget::updateState(bool running)
{
    m_startButton->setText(running ? "Pause" : Config::UiText::start);
}

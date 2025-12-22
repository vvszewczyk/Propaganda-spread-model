#include "WorldPhysicsWidget.hpp"

#include "UiUtils.hpp"

#include <QToolTip>

using namespace app::ui;

WorldPhysicsWidget::WorldPhysicsWidget(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 10, 5, 5);
    layout->setSpacing(8);

    // Sekcja 1: Podatność tłumu
    layout->addWidget(new QLabel("<b>Social Resilience</b>"));

    // Theta: 10-200 /100 => 0.10..2.00
    addParamSlider(layout, "Resistance (Theta)", m_slTheta, 10, 200, 100, 100.0, 2,
                   "Higher values make people harder to convince.");

    // Margin: 0-20 /100 => 0.00..0.20
    addParamSlider(layout, "Stability Margin", m_slMargin, 0, 20, 5, 100.0, 2,
                   "Prevents flickering when forces are equal.");

    // Sekcja 2: Histereza (Pamięć)
    layout->addSpacing(5);
    layout->addWidget(new QLabel("<b>Inertia & Memory</b>"));

    // Kappa: 0-50 /10 => 0.0..5.0
    addParamSlider(layout, "Loyalty (Kappa)", m_slKappa, 0, 50, 10, 10.0, 2,
                   "Multiplier for resistance when a side is already chosen.");

    // Grow/Decay: /1000 => 0.000..0.100
    addParamSlider(layout, "Memory Growth", m_slHysGrow, 0, 100, 20, 1000.0, 3,
                   "How fast loyalty grows when exposed to same narrative.");

    addParamSlider(layout, "Memory Decay", m_slHysDecay, 0, 100, 10, 1000.0, 3,
                   "How fast loyalty fades without reinforcement.");

    // Sekcja 3: Zaufanie do kanałów
    layout->addSpacing(5);
    layout->addWidget(new QLabel("<b>Channel Trust Weights</b>"));

    // Weights: /100 => 0.00..3.00
    addParamSlider(layout, "Local (Neighbors)", m_slWLocal, 0, 300, 100, 100.0, 2,
                   "Impact of neighbors.");

    addParamSlider(layout, "Broadcast (Media)", m_slWBroadcast, 0, 300, 100, 100.0, 2,
                   "Impact of global media.");

    addParamSlider(layout, "Social (Net)", m_slWSocial, 0, 300, 100, 100.0, 2,
                   "Impact of social network.");

    addParamSlider(layout, "Direct (Ads)", m_slWDM, 0, 300, 100, 100.0, 2,
                   "Impact of targeted campaigns.");
    layout->addStretch();
}

void WorldPhysicsWidget::addParamSlider(QVBoxLayout*   layout,
                                        const QString& name,
                                        QSlider*&      memberPtr,
                                        int            min,
                                        int            max,
                                        int            defaultVal,
                                        double         denom,
                                        int            decimals,
                                        const QString& tooltip)
{
    auto* lbl = new QLabel(name, this);
    lbl->setToolTip(tooltip);
    layout->addWidget(lbl);

    // wiersz: [slider...............] [value]
    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);

    memberPtr = makeWidget<QSlider>(this,
                                    [&](QSlider* s)
                                    {
                                        s->setOrientation(Qt::Horizontal);
                                        s->setRange(min, max);
                                        s->setValue(defaultVal);
                                        s->setToolTip(tooltip);

                                        // opcjonalnie: ticki
                                        s->setTickPosition(QSlider::TicksBelow);
                                        s->setTickInterval(std::max(1, (max - min) / 4));
                                    });

    auto* value = new QLabel(this);
    value->setToolTip(tooltip);
    value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    value->setFixedWidth(64);
    value->setStyleSheet("font-family: Consolas, monospace;");

    auto setValueText = [value, denom, decimals](int v)
    {
        const double scaled = static_cast<double>(v) / denom;
        value->setText(QString::number(scaled, 'f', decimals));
    };

    setValueText(defaultVal);

    row->addWidget(memberPtr, 1);
    row->addWidget(value, 0);
    layout->addLayout(row);

    m_bindings.push_back(Binding{memberPtr, value, denom, decimals});

    connect(memberPtr, &QSlider::valueChanged, this,
            [this, setValueText](int v)
            {
                setValueText(v);
                emit parametersChanged();
            });
}

BaseParameters WorldPhysicsWidget::getParameters() const
{
    BaseParameters p;

    p.thetaScale = static_cast<float>(m_slTheta->value()) / 100.0f;
    p.margin     = static_cast<float>(m_slMargin->value()) / 100.0f;

    p.switchKappa = static_cast<float>(m_slKappa->value()) / 10.0f;
    p.hysGrow     = static_cast<float>(m_slHysGrow->value()) / 1000.0f;
    p.hysDecay    = static_cast<float>(m_slHysDecay->value()) / 1000.0f;

    p.wLocal     = static_cast<float>(m_slWLocal->value()) / 100.0f;
    p.wBroadcast = static_cast<float>(m_slWBroadcast->value()) / 100.0f;
    p.wSocial    = static_cast<float>(m_slWSocial->value()) / 100.0f;
    p.wDM        = static_cast<float>(m_slWDM->value()) / 100.0f;

    return p;
}

void WorldPhysicsWidget::updateAllValueLabels()
{
    for (const auto& b : m_bindings)
    {
        if (!b.slider || !b.valueLabel)
            continue;
        const double scaled = static_cast<double>(b.slider->value()) / b.denom;
        b.valueLabel->setText(QString::number(scaled, 'f', b.decimals));
    }
}

void WorldPhysicsWidget::setParameters(const BaseParameters& p)
{
    const bool wasBlocked = this->blockSignals(true);

    m_slTheta->setValue(static_cast<int>(p.thetaScale * 100.0f));
    m_slMargin->setValue(static_cast<int>(p.margin * 100.0f));

    m_slKappa->setValue(static_cast<int>(p.switchKappa * 10.0f));
    m_slHysGrow->setValue(static_cast<int>(p.hysGrow * 1000.0f));
    m_slHysDecay->setValue(static_cast<int>(p.hysDecay * 1000.0f));

    m_slWLocal->setValue(static_cast<int>(p.wLocal * 100.0f));
    m_slWBroadcast->setValue(static_cast<int>(p.wBroadcast * 100.0f));
    m_slWSocial->setValue(static_cast<int>(p.wSocial * 100.0f));
    m_slWDM->setValue(static_cast<int>(p.wDM * 100.0f));

    updateAllValueLabels();

    this->blockSignals(wasBlocked);
}

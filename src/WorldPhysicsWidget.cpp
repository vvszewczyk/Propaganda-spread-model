#include "WorldPhysicsWidget.hpp"

#include "UiUtils.hpp"

#include <QFont>
#include <QLabel>
#include <QToolTip>

using namespace app::ui;

WorldPhysicsWidget::WorldPhysicsWidget(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 10, 5, 5);
    layout->setSpacing(8);

    auto addHeader = [&](const QString& text)
    {
        auto* lbl  = new QLabel(text, this);
        QFont font = lbl->font();
        font.setBold(true);
        font.setPointSize(font.pointSize() + 1);
        lbl->setFont(font);
        lbl->setStyleSheet("margin-top: 10px; margin-bottom: 2px;");
        layout->addWidget(lbl);
    };

    addHeader("Model Interactions");

    addParamSlider(layout, "thetaScale (Susceptibility)", m_thetaSlider, 20, 200, 130, 100.0, 2,
                   "Scales the cell threshold. Range: 0.2..2.0");

    addParamSlider(layout, "margin (Indifference Zone)", m_marginSlider, 0, 20, 6, 100.0, 2,
                   "Stability control: larger margin means fewer state flips. Range: 0..0.20");

    addHeader("Channel Weights");

    addParamSlider(layout, "wLocal (Neighbor Weight)", m_wLocalSlider, 0, 200, 55, 100.0, 2,
                   "Influence of neighbors (DM). Range: 0..2.0");

    addParamSlider(layout, "wDM (Self Confidence)", m_wDMSlider, 0, 100, 30, 100.0, 2,
                   "Confidence in own decision. Range: 0..1.0");

    addParamSlider(layout, "wBroadcast (Media Confidence)", m_wBroadcastSlider, 0, 100, 35, 100.0,
                   2, "Confidence in global broadcast/media. Range: 0..1.0");

    addParamSlider(layout, "wSocial (Social Network)", m_wSocialSlider, 0, 100, 12, 100.0, 2,
                   "Confidence in social connections. Range: 0..1.0 (Keep 0 if no graph)");

    addHeader("Broadcast Mechanics");

    addParamSlider(layout, "broadcastDecay (Exposure Decay)", m_broadcastDecaySlider, 0, 200, 15,
                   1000.0, 3, "How fast media topics are forgotten. Range: 0..0.200");

    addParamSlider(layout, "broadcastHysGain (Hysteresis Gain)", m_broadcastHysGain, 0, 50, 10,
                   1000.0, 3, "Reinforcement from media. Range: 0..0.050");

    addParamSlider(layout, "broadcastNeutralWeight", m_broadcastNeutralWeight, 0, 100, 30, 100, 2,
                   "How strongly media influences undecided cells.");

    addParamSlider(layout, "broadcastStockMax (Max Saturation)", m_broadcastStockMaxSlider, 50, 300,
                   160, 100.0, 2,
                   "Maximum accumulated media signal in the environment. Range: 0.5..3.0");

    addHeader("Open-Mindedness");

    addParamSlider(layout, "openMindDM", m_openMindLocalSlider, 0, 100, 55, 100.0, 2,
                   "How much opposing views from neighbors penetrate.\n0.0 = Echo chamber (block "
                   "all), 1.0 = Open mind.");

    addParamSlider(layout, "openMindSocial", m_openMindNetworkSlider, 0, 100, 60, 100.0, 2,
                   "How much opposing views from social network penetrate.\n0.0 = Block all, 1.0 = "
                   "Open mind.");

    addHeader("Hysteresis");

    addParamSlider(layout, "switchKappa (Resistance)", m_kappaSlider, 0, 200, 25, 100.0, 2,
                   "How hard it is to change side once decided. Range: 0..2.0");

    addParamSlider(layout, "hysDecay (Forgetting)", m_hysDecaySlider, 0, 50, 20, 1000.0, 3,
                   "Rate of opinion decay. Range: 0..0.050");

    addParamSlider(layout, "hysMaxTotal (Hysteresis Limit)", m_hysMaxTotalSlider, 0, 500, 140,
                   100.0, 2, "Maximum level of 'entrenchment'. Range: 0..5.0");

    addHeader("Hysteresis Dynamics (Gain/Erode)");

    addParamSlider(layout, "dmHysGain", m_dmHysGainSlider, 0, 100, 30, 1000.0, 3, "Range: 0..0.1");
    addParamSlider(layout, "dmHysErode", m_dmHysErodeSlider, 0, 100, 20, 1000.0, 3,
                   "Range: 0..0.1");

    addParamSlider(layout, "socialHysGain", m_socialHysGainSlider, 0, 100, 20, 1000.0, 3,
                   "Range: 0..0.1");
    addParamSlider(layout, "socialHysErode", m_socialHysErodeSlider, 0, 100, 15, 1000.0, 3,
                   "Range: 0..0.1");

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

    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);

    memberPtr = makeWidget<QSlider>(this,
                                    [&](QSlider* s)
                                    {
                                        s->setOrientation(Qt::Horizontal);
                                        s->setRange(min, max);
                                        s->setValue(defaultVal);
                                        s->setToolTip(tooltip);
                                        s->setTickPosition(QSlider::TicksBelow);
                                        s->setTickInterval(std::max(1, (max - min) / 4));
                                    });

    auto* value = new QLabel(this);
    value->setToolTip(tooltip);
    value->setAlignment(Qt::AlignRight bitor Qt::AlignVCenter);
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

    p.thetaScale = static_cast<float>(m_thetaSlider->value()) / 100.f;
    p.margin     = static_cast<float>(m_marginSlider->value()) / 100.f;
    p.wLocal     = static_cast<float>(m_wLocalSlider->value()) / 100.f;

    p.wDM        = static_cast<float>(m_wDMSlider->value()) / 100.f;
    p.wBroadcast = static_cast<float>(m_wBroadcastSlider->value()) / 100.f;
    p.wSocial    = static_cast<float>(m_wSocialSlider->value()) / 100.f;

    p.broadcastDecay         = static_cast<float>(m_broadcastDecaySlider->value()) / 1000.f;
    p.broadcastStockMax      = static_cast<float>(m_broadcastStockMaxSlider->value()) / 100.f;
    p.broadcastHysGain       = static_cast<float>(m_broadcastHysGain->value()) / 1000.f;
    p.broadcastNeutralWeight = static_cast<float>(m_broadcastNeutralWeight->value()) / 100.f;

    p.openMindDM     = static_cast<float>(m_openMindLocalSlider->value()) / 100.f;
    p.openMindSocial = static_cast<float>(m_openMindNetworkSlider->value()) / 100.f;

    p.switchKappa = static_cast<float>(m_kappaSlider->value()) / 100.f;
    p.hysDecay    = static_cast<float>(m_hysDecaySlider->value()) / 1000.f;
    p.hysMaxTotal = static_cast<float>(m_hysMaxTotalSlider->value()) / 100.f;

    p.dmHysGain      = static_cast<float>(m_dmHysGainSlider->value()) / 1000.f;
    p.dmHysErode     = static_cast<float>(m_dmHysErodeSlider->value()) / 1000.f;
    p.socialHysGain  = static_cast<float>(m_socialHysGainSlider->value()) / 1000.f;
    p.socialHysErode = static_cast<float>(m_socialHysErodeSlider->value()) / 1000.f;

    return p;
}

void WorldPhysicsWidget::updateAllValueLabels()
{
    for (const auto& b : m_bindings)
    {
        if (not b.slider or not b.valueLabel)
        {
            continue;
        }
        const double scaled = static_cast<double>(b.slider->value()) / b.denom;
        b.valueLabel->setText(QString::number(scaled, 'f', b.decimals));
    }
}

void WorldPhysicsWidget::setParameters(const BaseParameters& p)
{
    const bool wasBlocked = this->blockSignals(true);

    m_thetaSlider->setValue(static_cast<int>(p.thetaScale * 100.0f));
    m_marginSlider->setValue(static_cast<int>(p.margin * 100.0f));
    m_wLocalSlider->setValue(static_cast<int>(p.wLocal * 100.0f));

    m_wDMSlider->setValue(static_cast<int>(p.wDM * 100.0f));
    m_wBroadcastSlider->setValue(static_cast<int>(p.wBroadcast * 100.0f));
    m_wSocialSlider->setValue(static_cast<int>(p.wSocial * 100.0f));

    m_broadcastDecaySlider->setValue(static_cast<int>(p.broadcastDecay * 1000.0f));
    m_broadcastStockMaxSlider->setValue(static_cast<int>(p.broadcastStockMax * 100.0f));
    m_broadcastHysGain->setValue(static_cast<int>(p.broadcastHysGain * 1000.0f));
    m_broadcastNeutralWeight->setValue(static_cast<int>(p.broadcastNeutralWeight * 100.f));

    m_openMindLocalSlider->setValue(static_cast<int>(p.openMindDM * 100.f));
    m_openMindNetworkSlider->setValue(static_cast<int>(p.openMindSocial * 100.f));

    m_kappaSlider->setValue(static_cast<int>(p.switchKappa * 100.0f));
    m_hysDecaySlider->setValue(static_cast<int>(p.hysDecay * 1000.0f));
    m_hysMaxTotalSlider->setValue(static_cast<int>(p.hysMaxTotal * 100.0f));

    m_dmHysGainSlider->setValue(static_cast<int>(p.dmHysGain * 1000.f));
    m_dmHysErodeSlider->setValue(static_cast<int>(p.dmHysErode * 1000.f));
    m_socialHysGainSlider->setValue(static_cast<int>(p.socialHysGain * 1000.f));
    m_socialHysErodeSlider->setValue(static_cast<int>(p.socialHysErode * 1000.f));

    updateAllValueLabels();

    this->blockSignals(wasBlocked);
}

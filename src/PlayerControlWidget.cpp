#include "PlayerControlWidget.hpp"

#include "MainWindow.hpp"
#include "Model.hpp"
#include "UiUtils.hpp"

using namespace app::ui;

PlayerControlWidget::PlayerControlWidget(const QString& playerName, QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto* headerBox    = new QGroupBox(playerName);
    auto* headerLayout = new QVBoxLayout(headerBox);
    m_budgetLabel      = makeWidget<QLabel>(
        this, [](QLabel* label) { label->setStyleSheet("color: green; font-weight: bold;"); },
        Config::UiText::budget);
    m_costLabel = makeWidget<QLabel>(
        this, [](QLabel* label) { label->setStyleSheet("color: red;"); }, Config::UiText::cost);

    headerLayout->addWidget(m_budgetLabel);
    headerLayout->addWidget(m_costLabel);
    mainLayout->addWidget(headerBox);

    mainLayout->addWidget(createChannelGroup("Broadcast (TV/Media)", m_whiteBroadcastSlider,
                                             m_greyBroadcastSlider, m_blackBroadcastSlider));
    mainLayout->addWidget(createChannelGroup("Social Media (Network)", m_whiteSocialSlider,
                                             m_greySocialSlider, m_blackSocialSlider));
    mainLayout->addWidget(createChannelGroup("DM (Direct Message)", m_whiteDMSlider, m_greyDMSlider,
                                             m_blackDMSlider));
    mainLayout->addStretch();
}

QGroupBox* PlayerControlWidget::createChannelGroup(const QString& title,
                                                   QSlider*&      white,
                                                   QSlider*&      grey,
                                                   QSlider*&      black)
{
    auto* groupBox = new QGroupBox(title);
    auto* layout   = new QVBoxLayout(groupBox);
    layout->setSpacing(2);

    auto makeRow = [&](const QString& labelName, QSlider*& slider, const QString& color)
    {
        auto* row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);

        auto* label = new QLabel(labelName);
        label->setFixedWidth(40);

        slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 100);
        slider->setValue(0);

        slider->setStyleSheet(
            QString("QSlider::handle:horizontal { background-color: %1; border: 1px solid #5c5c5c; "
                    "width: 10px; margin: -2px 0; border-radius: 3px; }")
                .arg(color));

        auto* valueLbl = new QLabel("0.00");
        valueLbl->setFixedWidth(44);
        valueLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        valueLbl->setStyleSheet("color: gray; font-family: Consolas, monospace; font-size: 10px;");

        auto updateValue = [valueLbl](int v)
        {
            // wariant A
            valueLbl->setText(QString::number(static_cast<double>(v) / 100.0, 'f', 2));

            // wariant B
            // valueLbl->setText(QStringLiteral("%1%").arg(v, 3));
        };

        updateValue(slider->value());

        row->addWidget(label);
        row->addWidget(slider, 1);
        row->addWidget(valueLbl);
        layout->addLayout(row);

        connect(slider, &QSlider::valueChanged, this,
                [this, updateValue](int v)
                {
                    updateValue(v);
                    emit controlsChanged();
                });
    };

    makeRow("White", white, "#eeeeee");
    makeRow("Grey", grey, "#888888");
    makeRow("Black", black, "#222222");

    return groupBox;
}

Controls PlayerControlWidget::getControls() const
{
    Controls controls;
    // Mapping 0-100 -> 0.0-1.0
    auto value = [](QSlider* slider) { return static_cast<float>(slider->value()) / 100.0f; };

    controls.whiteBroadcast = value(m_whiteBroadcastSlider);
    controls.greyBroadcast  = value(m_greyBroadcastSlider);
    controls.blackBroadcast = value(m_blackBroadcastSlider);

    controls.whiteSocial = value(m_whiteSocialSlider);
    controls.greySocial  = value(m_greySocialSlider);
    controls.blackSocial = value(m_blackSocialSlider);

    controls.whiteDM = value(m_whiteDMSlider);
    controls.greyDM  = value(m_greyDMSlider);
    controls.blackDM = value(m_blackDMSlider);

    return controls;
}

void PlayerControlWidget::updateBudgetDisplay(float currentBudget, float plannedCost)
{
    m_budgetLabel->setText(
        QString("Budget: $%1").arg(static_cast<double>(currentBudget), 0, 'f', 2));
    m_costLabel->setText(
        QString("Planned Cost: $%1").arg(static_cast<double>(plannedCost), 0, 'f', 2));
}

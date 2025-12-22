#pragma once
#include <Model.hpp>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

class PlayerControlWidget : public QWidget
{
        Q_OBJECT
    public:
        explicit PlayerControlWidget(const QString& playerName, QWidget* parent = nullptr);

        [[nodiscard]] Controls getControls() const;

        void updateBudgetDisplay(float currentBudget, float plannedCost);
    signals:
        void controlsChanged();

    private:
        QGroupBox*
        createChannelGroup(const QString& title, QSlider*& white, QSlider*& grey, QSlider*& black);

        QSlider *m_whiteBroadcastSlider, *m_greyBroadcastSlider, *m_blackBroadcastSlider;
        QSlider *m_whiteSocialSlider, *m_greySocialSlider, *m_blackSocialSlider;
        QSlider *m_whiteDMSlider, *m_greyDMSlider, *m_blackDMSlider;

        QLabel* m_budgetLabel;
        QLabel* m_costLabel;
};

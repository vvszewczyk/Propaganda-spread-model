#pragma once

#include "Model.hpp"

#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

class WorldPhysicsWidget : public QWidget
{
        Q_OBJECT
    public:
        explicit WorldPhysicsWidget(QWidget* parent = nullptr);

        [[nodiscard]] BaseParameters getParameters() const;

        void setParameters(const BaseParameters& params);

    signals:
        void parametersChanged();

    private:
        struct Binding
        {
                QSlider* slider{nullptr};
                QLabel*  valueLabel{nullptr};
                double   denom{1.0};
                int      decimals{2};
        };

        std::vector<Binding> m_bindings;

        void addParamSlider(QVBoxLayout*   layout,
                            const QString& name,
                            QSlider*&      memberPtr,
                            int            min,
                            int            max,
                            int            defaultVal,
                            double         denom,
                            int            decimals,
                            const QString& tooltip);
        void updateAllValueLabels();

        QSlider* m_wLocalSlider{nullptr}; // Waga sąsiadów
        QSlider* m_thetaSlider{nullptr};  // Odporność
        QSlider* m_marginSlider{nullptr};

        QSlider* m_wDMSlider{nullptr};        // Zaufanie do reklam
        QSlider* m_wBroadcastSlider{nullptr}; // Zaufanie do mediów
        QSlider* m_wSocialSlider{nullptr};    // Zaufanie do internetu

        QSlider* m_kappaSlider{nullptr};       // Histereza (Bezwładność)
        QSlider* m_hysMaxTotalSlider{nullptr}; // clamp histerezy, żeby nie rosła w kosmos
        QSlider* m_hysDecaySlider{nullptr};    // Szybkość zapominania

        QSlider* m_broadcastDecaySlider{nullptr};
        QSlider* m_broadcastStockMaxSlider{nullptr};
        QSlider* m_broadcastHysGain{nullptr};
        QSlider* m_broadcastNeutralWeight{nullptr};
};

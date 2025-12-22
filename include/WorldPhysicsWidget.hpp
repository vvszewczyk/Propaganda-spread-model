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

        QSlider* m_slTheta{nullptr};    // Odporność
        QSlider* m_slMargin{nullptr};   // Odporność
        QSlider* m_slKappa{nullptr};    // Histereza (Bezwładność)
        QSlider* m_slHysGrow{nullptr};  // Szybkość utwierdzania
        QSlider* m_slHysDecay{nullptr}; // Szybkość zapominania

        QSlider* m_slWLocal{nullptr};     // Waga sąsiadów
        QSlider* m_slWBroadcast{nullptr}; // Zaufanie do mediów
        QSlider* m_slWSocial{nullptr};    // Zaufanie do internetu
        QSlider* m_slWDM{nullptr};        // Zaufanie do reklam
};


#include <QColor>
#include <QStringLiteral>
#include <QVector3D>
#include <qvectornd.h>

namespace Config
{
    namespace Grid
    {
        inline constexpr int pixelWidth  = 1260;
        inline constexpr int pixelHeight = 790;

        extern int cellSize;
        extern int gridCols;
        extern int gridRows;
    } // namespace Grid

    namespace Window
    {
        inline constexpr int width  = 1465;
        inline constexpr int height = 808;
    } // namespace Window

    namespace Layout
    {
        inline constexpr int margin            = 5;
        inline constexpr int mainMargin        = 0;
        inline constexpr int rowButtonsSpacing = 1;
        inline constexpr int leftPanelStretch  = 7;
        inline constexpr int rightPanelStretch = 1;
        inline constexpr int buttonWidth       = 50;
        inline constexpr int buttonHeight      = 25;
    } // namespace Layout

    namespace UiText
    {
        inline const QString simulationControl = QStringLiteral("Simulation control");
        inline const QString iterationPrefix   = QStringLiteral("Iteration: ");
        inline const QString start             = QStringLiteral("Start");
        inline const QString reset             = QStringLiteral("Reset");
        inline const QString step              = QStringLiteral("Step");
        inline const QString showStats         = QStringLiteral("Show stats");
        inline const QString showSimulation    = QStringLiteral("Show simulation");
        inline const QString showGrid          = QStringLiteral("Show grid");
        inline const QString neighbourhood     = QStringLiteral("Neighbourhood:");
        inline const QString vonNeumann        = QStringLiteral("Von Neumann");
        inline const QString moore             = QStringLiteral("Moore");
        inline const QString zoom              = QStringLiteral("Zoom: ");
        inline const QString fps               = QStringLiteral("FPS: ");
        inline const QString cellInfoPrefix    = QStringLiteral("Cell: ");
        inline const QString simulationSpeed   = QStringLiteral("Simulation speed:");
        inline const QString slower            = QStringLiteral("-");
        inline const QString faster            = QStringLiteral("+");
        inline const QString scenarioPreset    = QStringLiteral("Scenario preset:");
        inline const QString scenario1         = QStringLiteral("Scenario 1");
        inline const QString modelParameters   = QStringLiteral("Model parameters");
        inline const QString playerSettings    = QStringLiteral("Player Settings");
        inline const QString playerA           = QStringLiteral("Player A");
        inline const QString playerB           = QStringLiteral("Player B");
        inline const QString sideA             = QStringLiteral("Side A");
        inline const QString sideB             = QStringLiteral("Side B");
        inline const QString budget            = QStringLiteral("Budget: ");
        inline const QString cost              = QStringLiteral("Cost: ");
        inline const QString useMap            = QStringLiteral("Use map");
        inline const QString paintMode         = QStringLiteral("Paint mode");
        inline const QString physics           = QStringLiteral("Physics");
    } // namespace UiText

    namespace UiValues
    {
        inline constexpr double minSpin     = 0.0;
        inline constexpr double maxSpin     = 1.0;
        inline constexpr double stepSpin    = 0.01;
        inline constexpr double defaultSpin = 0.5;

    } // namespace UiValues

    namespace Colors
    {
        inline constexpr char centralWidgetColor[] = "background-color: #74f740";
        inline constexpr char leftBlockColor[]     = "background-color: #ffe4e1;";
        inline constexpr char rightBlockColor[]    = "background-color: #e1eaff;";
    } // namespace Colors

    namespace Timing
    {
        inline constexpr int simulationSpeed = 50;
    } // namespace Timing

    namespace Map
    {
        inline const QString usSvgPath = QStringLiteral("Propaganda-spread-model/map/us_test.svg");
    } // namespace Map

    namespace Simulation
    {
        constexpr float kEffWhite = 1.0f;
        constexpr float kEffGray  = 1.2f;
        constexpr float kEffBlack = 1.5f;
    } // namespace Simulation

    namespace Neighbourhood
    {
        inline const QVector2D VN[4] = {QVector2D{1, 0}, QVector2D{-1, 0}, QVector2D{0, 1},
                                        QVector2D{0, -1}};

        inline const QVector2D MOORE[8] = {QVector2D{-1, -1}, QVector2D{-1, 0}, QVector2D{-1, 1},
                                           QVector2D{0, -1},  QVector2D{0, 1},  QVector2D{1, -1},
                                           QVector2D{1, 0},   QVector2D{1, 1}};

    } // namespace Neighbourhood

    namespace GridWidget
    {
        inline constexpr QColor backgroundColor{144, 213, 255};
        inline constexpr int    gridAlpha = 40;
        inline constexpr int    tooltopMs = 3000;

        inline constexpr double zoomStep = 1.1;
        inline constexpr double zoomMin  = 0.2;
        inline constexpr double zoomMax  = 5.0;

        inline constexpr int frameThickness = 1;

    } // namespace GridWidget
} // namespace Config

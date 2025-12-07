
#include <QColor>
#include <qstringliteral.h>
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
        extern int gridDepth;
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
        inline constexpr int rowButtonsSpacing = 6;
        inline constexpr int leftPanelStretch  = 7;
        inline constexpr int rightPanelStretch = 1;
        inline constexpr int buttonWidth       = 50;
        inline constexpr int buttonHeight      = 20;
    } // namespace Layout

    namespace UiText
    {
        inline const QString simulationSettings = QStringLiteral("Simulation settings");
        inline const QString iterationPrefix    = QStringLiteral("Iteration: ");
        inline const QString start              = QStringLiteral("Start");
        inline const QString reset              = QStringLiteral("Reset");
        inline const QString showStats          = QStringLiteral("Show stats");
        inline const QString showSimulation     = QStringLiteral("Show simulation");
        inline const QString showGrid           = QStringLiteral("Show grid");
        inline const QString neighbourhood      = QStringLiteral("Neighbourhood");
        inline const QString vonNeumann         = QStringLiteral("Von Neumann");
        inline const QString moore              = QStringLiteral("Moore");
        inline const QString zoom               = QStringLiteral("Zoom: ");
        inline const QString fps                = QStringLiteral("FPS: ");
        inline const QString cellInfoPrefix     = QStringLiteral("Cell: ");
    } // namespace UiText

    namespace Colors
    {
        inline constexpr char centralWidgetColor[] = "background-color: #74f740";
        inline constexpr char leftBlockColor[]     = "background-color: #ffe4e1;";
        inline constexpr char rightBlockColor[]    = "background-color: #e1eaff;";
    } // namespace Colors

    namespace Timing
    {
        inline constexpr int simulationStepMs = 16;
    } // namespace Timing

    namespace Map
    {
        inline const QString usSvgPath = QStringLiteral("Propaganda-spread-model/map/us_test.svg");
    } // namespace Map

    namespace Simulation
    {
        extern int randomGrainNumber;
        extern int regularGrainStride;
    } // namespace Simulation

    namespace Neighbourhood
    {
        inline const QVector3D VN[6] = {QVector3D(1, 0, 0), QVector3D(-1, 0, 0),
                                        QVector3D(0, 1, 0), QVector3D(0, -1, 0),
                                        QVector3D(0, 0, 1), QVector3D(0, 0, -1)};

        inline const QVector3D MOORE[26] = {
            QVector3D(-1, -1, -1), QVector3D(-1, -1, 0), QVector3D(-1, -1, 1), QVector3D(-1, 0, -1),
            QVector3D(-1, 0, 0),   QVector3D(-1, 0, 1),  QVector3D(-1, 1, -1), QVector3D(-1, 1, 0),
            QVector3D(-1, 1, 1),   QVector3D(0, -1, -1), QVector3D(0, -1, 0),  QVector3D(0, -1, 1),
            QVector3D(0, 0, -1),   QVector3D(0, 0, 1),   QVector3D(0, 1, -1),  QVector3D(0, 1, 0),
            QVector3D(0, 1, 1),    QVector3D(1, -1, -1), QVector3D(1, -1, 0),  QVector3D(1, -1, 1),
            QVector3D(1, 0, -1),   QVector3D(1, 0, 0),   QVector3D(1, 0, 1),   QVector3D(1, 1, -1),
            QVector3D(1, 1, 0),    QVector3D(1, 1, 1)};
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

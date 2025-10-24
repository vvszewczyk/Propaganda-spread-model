
#include <qvectornd.h>
namespace Config
{
    // Grid
    const int  gridPixelWidth  = 1260;
    const int  gridPixelHeight = 790;
    extern int cellSize;
    extern int gridCols;
    extern int gridRows;
    extern int gridDepth;

    // Main window
    const int windowWidth  = 1465;
    const int windowHeight = 808;

    // Buttons
    const int buttonWidth  = 50;
    const int buttonHeight = 20;

    // Simulation
    extern int randomGrainNumber;
    extern int regularGrainStride;

    // Panel colors
    const char centralWidgetColor[] = "background-color: #74f740";  // lightgreen
    const char leftBlockColor[]     = "background-color: #ffe4e1;"; // lightpink
    const char rightBlockColor[]    = "background-color: #e1eaff;"; // lightblue

    // Margin/padding
    const int margin = 5;

    // Neighbourhood offsets
    static const QVector3D VN[6] = {QVector3D(1, 0, 0),  QVector3D(-1, 0, 0), QVector3D(0, 1, 0),
                                    QVector3D(0, -1, 0), QVector3D(0, 0, 1),  QVector3D(0, 0, -1)};

    static const QVector3D MOORE[26] = {
        QVector3D(-1, -1, -1), QVector3D(-1, -1, 0), QVector3D(-1, -1, 1), QVector3D(-1, 0, -1),
        QVector3D(-1, 0, 0),   QVector3D(-1, 0, 1),  QVector3D(-1, 1, -1), QVector3D(-1, 1, 0),
        QVector3D(-1, 1, 1),   QVector3D(0, -1, -1), QVector3D(0, -1, 0),  QVector3D(0, -1, 1),
        QVector3D(0, 0, -1),   QVector3D(0, 0, 1),   QVector3D(0, 1, -1),  QVector3D(0, 1, 0),
        QVector3D(0, 1, 1),    QVector3D(1, -1, -1), QVector3D(1, -1, 0),  QVector3D(1, -1, 1),
        QVector3D(1, 0, -1),   QVector3D(1, 0, 0),   QVector3D(1, 0, 1),   QVector3D(1, 1, -1),
        QVector3D(1, 1, 0),    QVector3D(1, 1, 1)};

    static const QVector3D HEXAGON_A[6] = {QVector3D(-1, -1, 0), QVector3D(0, -1, 0),
                                           QVector3D(1, 0, 0),   QVector3D(0, 1, 0),
                                           QVector3D(-1, 0, 0),  QVector3D(1, 1, 0)};

    static const QVector3D HEXAGON_B[6] = {QVector3D(0, -1, 0), QVector3D(1, -1, 0),
                                           QVector3D(1, 0, 0),  QVector3D(0, 1, 0),
                                           QVector3D(-1, 0, 0), QVector3D(-1, 1, 0)};
} // namespace Config

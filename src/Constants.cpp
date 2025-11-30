#include "Constants.hpp"

#include "GridWidget.hpp"

namespace Config
{
    namespace Grid
    {
        int cellSize  = 1;
        int gridCols  = static_cast<int>(std::floor(pixelWidth / double(cellSize)));
        int gridRows  = static_cast<int>(std::floor(pixelHeight / double(cellSize)));
        int gridDepth = 50;
    } // namespace Grid

    namespace Simulation
    {
        int randomGrainNumber  = 500;
        int regularGrainStride = 10;
    } // namespace Simulation
} // namespace Config

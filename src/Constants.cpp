#include "Constants.hpp"

#include "GridWidget.hpp"

namespace Config
{
    int cellSize           = 1;
    int gridCols           = static_cast<int>(std::floor(gridPixelWidth / double(cellSize)));
    int gridRows           = static_cast<int>(std::floor(gridPixelHeight / double(cellSize)));
    int gridDepth          = 50;
    int randomGrainNumber  = 500;
    int regularGrainStride = 10;
} // namespace Config

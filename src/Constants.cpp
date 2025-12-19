#include "Constants.hpp"

#include "GridWidget.hpp"

namespace Config
{
    namespace Grid
    {
        int cellSize = 1;
        int gridCols = static_cast<int>(std::floor(pixelWidth / double(cellSize)));
        int gridRows = static_cast<int>(std::floor(pixelHeight / double(cellSize)));
    } // namespace Grid
} // namespace Config

#pragma once
#include <cstdint>

enum class Side : uint8_t
{
    NONE,
    A,
    B
};

enum class NeighbourhoodType : uint8_t
{
    VON_NEUMANN = 0,
    MOORE       = 1

};

struct CellData
{
        Side side   = Side::NONE;
        bool active = true;

        double threshold  = 0.5;
        double hysteresis = 0.0;

        uint8_t stateId = 0;
};

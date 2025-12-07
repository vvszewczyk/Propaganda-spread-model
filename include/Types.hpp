#pragma once
#include <cstdint>

enum class State : uint8_t
{
    S,
    E,
    I,
    R,
    D
};

enum class Side : uint8_t
{
    NONE,
    A,
    B
};

enum class NeighbourhoodType : uint8_t
{
    MOORE,
    VON_NEUMANN
};

struct CellData
{
        State             state;
        Side              side;
        NeighbourhoodType neighbourhoodType;
        double            resilience = 0.0;
        double            fatigue    = 0.0;
        bool              active     = true;
        uint8_t           stateId    = 0;
};

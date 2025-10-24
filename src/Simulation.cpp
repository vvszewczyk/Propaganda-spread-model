#include "Simulation.hpp"

#include "Constants.hpp"

Simulation::Simulation(int cols, int rows)
    : m_cols{cols},
      m_rows{rows},
      m_currentGrid{static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows)},
      m_nextGrid{static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows)}
{
}

void Simulation::setParameters(const BaseParameters& params)
{
    m_parameters = params;
}

void Simulation::setPlayers(const Player& A, const Player& B)
{
    m_playerA = A;
    m_playerB = B;
}

void Simulation::setPools(const Pools& pools)
{
    m_pools = pools;
}

void Simulation::setNeighbourhoodType(NeighbourhoodType type)
{
    m_neighbourhoodType = type;
}

void Simulation::seedRandomly(int countA, int countB) {};

void Simulation::reset()
{
    m_currentGrid =
        std::vector<CellData>(static_cast<std::size_t>(m_cols) * static_cast<std::size_t>(m_rows));
    m_nextGrid =
        std::vector<CellData>(static_cast<std::size_t>(m_cols) * static_cast<std::size_t>(m_rows));
}

void Simulation::step()
{
}

int Simulation::getCols() const
{
    return m_cols;
}

int Simulation::getRows() const
{
    return m_rows;
}

const CellData& Simulation::cell(int x, int y) const
{
    std::vector<CellData>::size_type index =
        static_cast<std::vector<CellData>::size_type>(y) *
            static_cast<std::vector<CellData>::size_type>(m_cols) +
        static_cast<std::vector<CellData>::size_type>(x);
    return m_currentGrid.at(index);
}

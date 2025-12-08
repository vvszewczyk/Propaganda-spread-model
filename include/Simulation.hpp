#pragma once
#include "Model.hpp"

#include <vector>

class Simulation
{
    public:
        Simulation(int cols, int rows);
        void setParameters(const BaseParameters&);
        void setPlayers(const Player& A, const Player& B);
        void setPools(const Pools&);
        void setNeighbourhoodType(NeighbourhoodType type);

        [[nodiscard]] int getIteration() const;

        void seedRandomly(int countA, int countB);
        void reset();

        void step();

        [[nodiscard]] int             getCols() const;
        [[nodiscard]] int             getRows() const;
        [[nodiscard]] const CellData& cell(int x, int y) const;

    private:
        int                   m_cols;
        int                   m_rows;
        std::vector<CellData> m_currentGrid;
        std::vector<CellData> m_nextGrid;
        int                   m_iteration{0};

        BaseParameters    m_parameters{};
        Player            m_playerA{};
        Player            m_playerB{};
        Pools             m_pools{};
        NeighbourhoodType m_neighbourhoodType{};
};

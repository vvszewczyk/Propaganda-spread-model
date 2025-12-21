#pragma once
#include "Model.hpp"
#include "Types.hpp"

#include <random>
#include <vector>

class Simulation
{
    public:
        Simulation(int cols, int rows);

        void setParameters(const BaseParameters&);
        void setPlayers(const Player& A, const Player& B);
        void setNeighbourhoodType(NeighbourhoodType type);
        void reset();
        void seedRandomly(int countA, int countB);
        void setAllThreshold(double);

        void step();

        [[nodiscard]] int getIteration() const;
        [[nodiscard]] int getCols() const;
        [[nodiscard]] int getRows() const;

        [[nodiscard]] CellData&       cellAt(int x, int y);
        [[nodiscard]] const CellData& cellAt(int x, int y) const;

    private:
        [[nodiscard]] GlobalSignals calculateCampaignImpact();
        [[nodiscard]] float         calculateLocalPressure(int x, int y) const;
        void updateCellState(const CellData& currentCell, CellData& nextCell, float h);
        static float
        calculateEffectiveSignal(const Controls& controls, float scale, const Player& player);

    private:
        int                   m_cols;
        int                   m_rows;
        std::vector<CellData> m_currentGrid;
        std::vector<CellData> m_nextGrid;
        int                   m_iteration{0};

        BaseParameters    m_parameters{};
        Player            m_playerA{};
        Player            m_playerB{};
        NeighbourhoodType m_neighbourhoodType{};

        std::mt19937 m_rng;
};

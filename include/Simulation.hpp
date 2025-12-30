#pragma once
#include "Model.hpp"
#include "SimulationResults.hpp"
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
        void setThresholdRandomly();

        void step();

        [[nodiscard]] int getIteration() const;
        [[nodiscard]] int getCols() const;
        [[nodiscard]] int getRows() const;

        [[nodiscard]] const StepStats& getlastStepStats() const;

        [[nodiscard]] Player getPlayerA() const;
        [[nodiscard]] Player getPlayerB() const;

        [[nodiscard]] CellData&       cellAt(int x, int y);
        [[nodiscard]] const CellData& cellAt(int x, int y) const;

    private:
        [[nodiscard]] GlobalSignals calculateCampaignImpact(CampaignDiag& outDiag);

        [[nodiscard]] float calculateNeighbourInfluence(int x, int y) const;
        [[nodiscard]] float
        calculateDMInfluence(int x, int y, const GlobalSignals& globalSignals) const;
        [[nodiscard]] float calculateSocialInfluence(std::size_t i) const;
        [[nodiscard]] float applyBroadcastPersuasionForNeutrals(
            const CellData& currentCell, float baseH, const GlobalSignals& globalSignals) const;

        void applyBroadcastReinforcementForSupporters(const CellData&      currentCell,
                                                      CellData&            nextCell,
                                                      const GlobalSignals& globalSignals) const;
        void buildSocialNetwork(float rewiringProb);
        void applyChannelHysteresis(const CellData& currentCell,
                                    CellData&       nextCell,
                                    float           perceivedSignal,
                                    float           gain,
                                    float           erode,
                                    float           hysMax);
        void updateCellState(const CellData& currentCell, CellData& nextCell, float h);

    private:
        int                   m_cols;
        int                   m_rows;
        std::vector<CellData> m_currentGrid;
        std::vector<CellData> m_nextGrid;
        int                   m_iteration{};

        StepStats m_lastStepStats{};

        BaseParameters m_parameters{};
        Player         m_playerA{};
        Player         m_playerB{};

        float m_broadcastStockA = 0.0f;
        float m_broadcastStockB = 0.0f;

        std::vector<std::vector<std::size_t>> m_socialGraph;

        NeighbourhoodType m_neighbourhoodType{};

        std::mt19937 m_rng;
};

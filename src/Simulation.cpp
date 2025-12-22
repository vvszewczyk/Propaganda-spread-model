#include "Simulation.hpp"

#include "Constants.hpp"
#include "Model.hpp"
#include "Types.hpp"

#include <algorithm>
#include <random>
#include <span>

namespace
{
    inline float getSideScalar(Side side)
    {
        switch (side)
        {
        case Side::A:
            return 1.0f;
        case Side::B:
            return -1.0f;
        default:
            return 0.0f;
        }
    }

    inline float computeChannelStrength(float white, float grey, float black)
    {
        return white * Config::Simulation::kEffWhite + grey * Config::Simulation::kEffGray +
               black * Config::Simulation::kEffBlack;
    }

    inline float clamp(float x)
    {
        return std::max(0.0f, std::min(1.0f, x));
    }
} // namespace

Simulation::Simulation(int cols, int rows)
    : m_cols{cols},
      m_rows{rows},
      m_currentGrid{static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows)},
      m_nextGrid{static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows)},
      m_rng{std::random_device{}()}
{
    this->seedRandomly(500, 500);
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

void Simulation::setNeighbourhoodType(NeighbourhoodType type)
{
    m_neighbourhoodType = type;
}

int Simulation::getIteration() const
{
    return m_iteration;
}

int Simulation::getCols() const
{
    return m_cols;
}

int Simulation::getRows() const
{
    return m_rows;
}

Player Simulation::getPlayerA() const
{
    return m_playerA;
}

Player Simulation::getPlayerB() const
{
    return m_playerB;
}

void Simulation::reset()
{
    m_currentGrid =
        std::vector<CellData>(static_cast<std::size_t>(m_cols) * static_cast<std::size_t>(m_rows));
    m_nextGrid =
        std::vector<CellData>(static_cast<std::size_t>(m_cols) * static_cast<std::size_t>(m_rows));
    m_iteration = 0;
    seedRandomly(500, 500);
    setAllThreshold(0.3);
}

CellData& Simulation::cellAt(int x, int y)
{
    std::vector<CellData>::size_type index =
        static_cast<std::vector<CellData>::size_type>(y) *
            static_cast<std::vector<CellData>::size_type>(m_cols) +
        static_cast<std::vector<CellData>::size_type>(x);
    return m_currentGrid.at(index);
}
const CellData& Simulation::cellAt(int x, int y) const
{
    std::vector<CellData>::size_type index =
        static_cast<std::vector<CellData>::size_type>(y) *
            static_cast<std::vector<CellData>::size_type>(m_cols) +
        static_cast<std::vector<CellData>::size_type>(x);
    return m_currentGrid.at(index);
}

void Simulation::setAllThreshold(double t)
{
    for (auto& c : m_currentGrid)
        c.threshold = t;
    for (auto& c : m_nextGrid)
        c.threshold = t;
}

void Simulation::seedRandomly(int countA, int countB)
{
    std::mt19937                       rng{std::random_device{}()};
    std::uniform_int_distribution<int> distX(0, m_cols - 1);
    std::uniform_int_distribution<int> distY(0, m_rows - 1);

    auto place = [&](Side side, int count)
    {
        int placed = 0;
        while (placed < count)
        {
            int x = distX(rng);
            int y = distY(rng);

            CellData& cell = cellAt(x, y);
            if (not cell.active)
            {
                continue;
            }

            if (cell.side != Side::NONE)
            {
                continue;
            }

            cell.side       = side;
            cell.hysteresis = 0.0;
            ++placed;
        }
    };
    place(Side::A, countA);
    place(Side::B, countB);
}

GlobalSignals Simulation::calculateCampaignImpact()
{
    // Calculate how much players want to spend
    float costA = m_playerA.calculatePlannedCost();
    float costB = m_playerB.calculatePlannedCost();

    // Scale budget if they are too poor for action
    float scaleA =
        (costA > 0 and m_playerA.budget > 0) ? std::min(1.0f, m_playerA.budget / costA) : 0.0f;
    float scaleB =
        (costB > 0 and m_playerB.budget > 0) ? std::min(1.0f, m_playerB.budget / costB) : 0.0f;

    m_playerA.budget -= costA * scaleA;
    m_playerB.budget -= costB * scaleB;

    GlobalSignals globalSignals;
    const auto&   controlsA = m_playerA.controls;
    const auto&   controlsB = m_playerB.controls;

    // clang-format off
    // Broadcast
    const float bA = computeChannelStrength(controlsA.whiteBroadcast,
                                             controlsA.greyBroadcast,
                                            controlsA.blackBroadcast) * scaleA;
    const float bB = computeChannelStrength(controlsB.whiteBroadcast,
                                             controlsB.greyBroadcast,
                                            controlsB.blackBroadcast) * scaleB;
    globalSignals.broadcastPressure = m_parameters.wBroadcast * (bA - bB);

    // Social
    float sA = computeChannelStrength(controlsA.whiteSocial,
                                       controlsA.greySocial,
                                      controlsA.blackSocial) * scaleA;
    float sB = computeChannelStrength(controlsB.whiteSocial,
                                       controlsB.greySocial,
                                      controlsB.blackSocial) * scaleB;
    globalSignals.socialPressure = m_parameters.wSocial * (sA - sB);

    // DM
    float dA =
        computeChannelStrength(controlsA.whiteDM,
                                controlsA.greyDM,
                               controlsA.blackDM) * scaleA;
    float dB =
        computeChannelStrength(controlsB.whiteDM,
                                controlsB.greyDM,
                               controlsB.blackDM) * scaleB;
    globalSignals.dmPressure = m_parameters.wDM * (dA - dB);

    // clang-format on
    return globalSignals;
}

float Simulation::calculateLocalPressure(int x, int y) const
{
    auto idx = [&](int xn, int yn) -> std::size_t
    {
        return static_cast<std::size_t>(yn) * static_cast<std::size_t>(m_cols) +
               static_cast<std::size_t>(xn);
    };

    float hDM                 = 0.0f;
    int   count               = 0;
    auto  accumulateNeighbour = [&](int nx, int ny)
    {
        if (nx < 0 || ny < 0 || nx >= m_cols || ny >= m_rows)
        {
            return;
        }
        const CellData& neighborCell = m_currentGrid[idx(nx, ny)];
        if (not neighborCell.active)
        {
            return;
        }
        if (neighborCell.side != Side::NONE)
        {
            hDM += getSideScalar(neighborCell.side);
            ++count;
        }
    };

    std::span<const QVector2D> offsets =
        (m_neighbourhoodType == NeighbourhoodType::MOORE)
            ? std::span<const QVector2D>(Config::Neighbourhood::MOORE)
            : std::span<const QVector2D>(Config::Neighbourhood::VN);

    for (const auto& offset : offsets)
    {
        accumulateNeighbour(x + static_cast<int>(offset.x()), y + static_cast<int>(offset.y()));
    }

    return (count == 0) ? 0.0f : hDM / static_cast<float>(count);
}

void Simulation::updateCellState(const CellData& currentCell, CellData& nextCell, float h)
{
    const float theta  = static_cast<float>(currentCell.threshold) * m_parameters.thetaScale;
    const float margin = m_parameters.margin;

    if (currentCell.side == Side::NONE)
    {
        if (h >= theta + margin)
        {
            nextCell.side = Side::A;
        }
        else if (h <= -(theta + margin))
        {
            nextCell.side = Side::B;
        }

        // If neutral -> hysteresis decrease
        nextCell.hysteresis =
            std::max(0.0, currentCell.hysteresis - static_cast<double>(m_parameters.hysDecay));
    }
    else
    {
        const float resistance =
            1.0f + m_parameters.switchKappa * static_cast<float>(currentCell.hysteresis);
        const float effectiveTheta = theta * resistance;

        if (currentCell.side == Side::A)
        {
            if (h <= -(effectiveTheta + margin))
            {
                nextCell.side       = Side::B;
                nextCell.hysteresis = 0.0;
            }
            else
            {
                if (h > 0)
                {
                    nextCell.hysteresis =
                        currentCell.hysteresis + static_cast<double>(m_parameters.hysGrow);
                }
            }
        }
        else if (currentCell.side == Side::B)
        {
            if (h >= effectiveTheta + margin)
            {
                nextCell.side       = Side::A;
                nextCell.hysteresis = 0.0;
            }
            else
            {
                if (h < 0)
                {
                    nextCell.hysteresis =
                        currentCell.hysteresis + static_cast<double>(m_parameters.hysGrow);
                }
            }
        }
    }
}

void Simulation::step()
{
    m_nextGrid                        = m_currentGrid;
    const GlobalSignals globalSignals = calculateCampaignImpact();

    auto idx = [&](int x, int y)
    {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(m_cols) +
               static_cast<std::size_t>(x);
    };

    for (int y = 0; y < m_rows; ++y)
    {
        for (int x = 0; x < m_cols; ++x)
        {
            const std::size_t i           = idx(x, y);
            const CellData&   currentCell = m_currentGrid[i];
            CellData&         nextCell    = m_nextGrid[i];

            if (not currentCell.active)
            {
                continue;
            }

            const float localPressure = calculateLocalPressure(x, y);
            float totalInfluence      = (localPressure * m_parameters.wLocal) + globalSignals.sum();
            updateCellState(currentCell, nextCell, totalInfluence);
        }
    }
    m_currentGrid.swap(m_nextGrid);
    ++m_iteration;
}

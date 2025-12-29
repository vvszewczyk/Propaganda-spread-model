#include "Simulation.hpp"

#include "Constants.hpp"
#include "Model.hpp"
#include "Types.hpp"

#include <QDebug>
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
} // namespace

Simulation::Simulation(int cols, int rows)
    : m_cols{cols},
      m_rows{rows},
      m_currentGrid{static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows)},
      m_nextGrid{static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows)},
      m_rng{std::random_device{}()}
{
    seedRandomly(500, 500);
    buildSocialNetwork(0.05f);
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
    buildSocialNetwork(0.05f);
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

    m_broadcastStockA = 0.0f;
    m_broadcastStockB = 0.0f;

    seedRandomly(500, 500);
    buildSocialNetwork(0.05f);
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

void Simulation::setThresholdRandomly()
{
    std::uniform_real_distribution<double> distTheta(0.05, 0.6);

    for (auto& cell : m_currentGrid)
    {
        cell.threshold = distTheta(m_rng);
    }
    for (std::size_t i = 0; i < m_nextGrid.size(); ++i)
    {
        m_nextGrid[i].threshold = m_currentGrid[i].threshold;
    }
}

void Simulation::seedRandomly(int countA, int countB)
{
    std::uniform_int_distribution<int> distX(0, m_cols - 1);
    std::uniform_int_distribution<int> distY(0, m_rows - 1);

    auto place = [&](Side side, int count)
    {
        int placed = 0;
        while (placed < count)
        {
            int x = distX(m_rng);
            int y = distY(m_rng);

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

    setThresholdRandomly();
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
    //globalSignals.broadcastPressure = m_parameters.wBroadcast * (bA - bB);

    m_broadcastStockA *= (1.0f - m_parameters.broadcastDecay);
    m_broadcastStockB *= (1.0f - m_parameters.broadcastDecay);
    m_broadcastStockA += bA;
    m_broadcastStockB += bB;
    m_broadcastStockA = std::clamp(m_broadcastStockA, 0.0f, m_parameters.broadcastStockMax);
    m_broadcastStockB = std::clamp(m_broadcastStockB, 0.0f, m_parameters.broadcastStockMax);
    globalSignals.broadcastA = m_parameters.wBroadcast * m_broadcastStockA;
    globalSignals.broadcastB = m_parameters.wBroadcast * m_broadcastStockB;


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

float Simulation::calculateNeighbourInfluence(int x, int y) const
{
    auto idx = [&](int xn, int yn)
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

float Simulation::calculateDMInfluence(int x, int y, const GlobalSignals& globalSignals) const
{
    const float neighborsInfluence = calculateNeighbourInfluence(x, y);
    return (neighborsInfluence * m_parameters.wLocal) + globalSignals.dmPressure;
}

float Simulation::calculateSocialInfluence(std::size_t i) const
{
    if (i >= m_socialGraph.size())
    {
        return 0.0f;
    }

    float hSocial = 0.0f;
    int   count   = 0;

    for (std::size_t neighbor : m_socialGraph[i])
    {
        if (not m_currentGrid[neighbor].active)
        {
            continue;
        }
        if (m_currentGrid[neighbor].side == Side::NONE)
        {
            continue;
        }

        hSocial += getSideScalar(m_currentGrid[neighbor].side);
        ++count;
    }

    return (count == 0) ? 0.0f : (hSocial / static_cast<float>(count));
}

float Simulation::applyBroadcastPersuasionForNeutrals(const CellData&      currentCell,
                                                      float                baseH,
                                                      const GlobalSignals& globalSignals) const
{
    if (currentCell.side == Side::NONE)
    {
        const float stockMax   = std::max(1e-6f, m_parameters.broadcastStockMax);
        float       biasNorm   = globalSignals.broadcastBias() / stockMax;
        biasNorm               = std::clamp(biasNorm, -1.0f, 1.0f);
        constexpr float kAlpha = 3.0f; // 2.0 - 6.0
        const float     shaped = std::tanh(kAlpha * biasNorm);
        return baseH + (m_parameters.broadcastNeutralWeight * shaped);
    }

    return baseH;
}

void Simulation::applyBroadcastReinforcementForSupporters(const CellData&      currentCell,
                                                          CellData&            nextCell,
                                                          const GlobalSignals& globalSignals) const
{
    if (nextCell.side != Side::NONE and nextCell.side == currentCell.side)
    {
        const float chosenSignalStrength =
            (nextCell.side == Side::A) ? globalSignals.broadcastA : globalSignals.broadcastB;

        const double boost =
            static_cast<double>(m_parameters.broadcastHysGain * chosenSignalStrength);

        nextCell.hysteresis = std::min<double>(static_cast<double>(m_parameters.broadcastHysMax),
                                               nextCell.hysteresis + boost);
    }
}

void Simulation::buildSocialNetwork(float rewiringProb)
{
    std::size_t totalCells = static_cast<std::size_t>(m_cols) * static_cast<std::size_t>(m_rows);
    m_socialGraph.assign(totalCells, {});

    std::span<const QVector2D> offsets =
        (m_neighbourhoodType == NeighbourhoodType::MOORE)
            ? std::span<const QVector2D>(Config::Neighbourhood::MOORE)
            : std::span<const QVector2D>(Config::Neighbourhood::VN);

    std::uniform_real_distribution<float>      dis(0.0f, 1.0f);
    std::uniform_int_distribution<std::size_t> randomCell(0, totalCells - 1);

    auto idx = [&](int x, int y)
    {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(m_cols) +
               static_cast<std::size_t>(x);
    };

    auto wrap = [&](int index, int range)
    {
        int result = index % range;
        if (result < 0)
        {
            result += range;
        }
        return result;
    };

    auto applyRewiringRule = [&](int x, int y, std::size_t i)
    {
        std::size_t neighbourId = idx(x, y);
        if (not m_currentGrid[neighbourId].active)
        {
            return;
        }

        if (dis(m_rng) < rewiringProb)
        {
            std::size_t k = neighbourId;
            do
            {
                k = randomCell(m_rng);
            } while (k == neighbourId or not m_currentGrid[k].active);

            neighbourId = k;
        }

        if (std::find(m_socialGraph[i].begin(), m_socialGraph[i].end(), neighbourId) ==
            m_socialGraph[i].end())
        {
            m_socialGraph[i].push_back(neighbourId);
        }

        if (std::find(m_socialGraph[neighbourId].begin(), m_socialGraph[neighbourId].end(), i) ==
            m_socialGraph[neighbourId].end())
        {
            m_socialGraph[neighbourId].push_back(i);
        }
    };

    for (int y = 0; y < m_rows; ++y)
    {
        for (int x = 0; x < m_cols; ++x)
        {
            const std::size_t i = idx(x, y);
            if (not m_currentGrid[i].active)
            {
                continue;
            }

            for (const auto& offset : offsets)
            {
                const int nx = wrap(x + static_cast<int>(offset.x()), m_cols);
                const int ny = wrap(y + static_cast<int>(offset.y()), m_rows);
                applyRewiringRule(nx, ny, i);
            }
        }
    }
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

            const float dmPressure = calculateDMInfluence(x, y, globalSignals);
            const float socialPressure =
                m_parameters.wSocial * calculateSocialInfluence(i) + globalSignals.socialPressure;
            const float baseH = dmPressure + socialPressure;

            const float h = applyBroadcastPersuasionForNeutrals(currentCell, baseH, globalSignals);

            updateCellState(currentCell, nextCell, h);

            applyBroadcastReinforcementForSupporters(currentCell, nextCell, globalSignals);
        }
    }
    m_currentGrid.swap(m_nextGrid);
    if (m_iteration % 50 == 0)
    {
        qDebug().noquote() << "iter" << m_iteration << "\n"
                           << " Broadcast:\n"
                           << "  decay =" << m_parameters.broadcastDecay << "\n"
                           << "  neutralWeight =" << m_parameters.broadcastNeutralWeight << "\n"
                           << "  hysGain =" << m_parameters.broadcastHysGain << "\n"
                           << "  hysMax =" << m_parameters.broadcastHysMax << "\n"
                           << "  stockMax =" << m_parameters.broadcastStockMax << "\n"
                           << " Weights:\n"
                           << "  wBroadcast =" << m_parameters.wBroadcast << "\n"
                           << "  wSocial =" << m_parameters.wSocial << "\n"
                           << "  wDM =" << m_parameters.wDM << "\n"
                           << "  wLocal =" << m_parameters.wLocal << "\n"
                           << " Decision:\n"
                           << "  thetaScale =" << m_parameters.thetaScale << "\n"
                           << "  margin =" << m_parameters.margin << "\n"
                           << " Hysteresis:\n"
                           << "  switchKappa =" << m_parameters.switchKappa << "\n"
                           << "  hysGrow =" << m_parameters.hysGrow << "\n"
                           << "  hysDecay =" << m_parameters.hysDecay;
    }
    ++m_iteration;
}

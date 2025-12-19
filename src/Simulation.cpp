#include "Simulation.hpp"

#include "Constants.hpp"
#include "Model.hpp"
#include "Types.hpp"

#include <algorithm>
#include <random>

Simulation::Simulation(int cols, int rows)
    : m_cols{cols},
      m_rows{rows},
      m_currentGrid{static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows)},
      m_nextGrid{static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows)},
      m_iteration{0}
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

namespace
{
    inline int spin(Side side)
    {
        switch (side)
        {
        case Side::A:
            return +1;
        case Side::B:
            return -1;
        default:
            return 0;
        }
    }

    inline float channelEff(float white, float grey, float black)
    {
        return white * Config::Simulation::kEffWhite + grey * Config::Simulation::kEffGray +
               black * Config::Simulation::kEffBlack;
    }

    inline float clamp(float x)
    {
        return std::max(0.0f, std::min(1.0f, x));
    }
} // namespace

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

static float computeSpendCost(const Player& player)
{
    const auto& controls = player.controls;
    const float sumWhite = controls.whiteBroadcast + controls.whiteSocial + controls.whiteDM;
    const float sumGrey  = controls.greyBroadcast + controls.greySocial + controls.greyDM;
    const float sumBlack = controls.blackBroadcast + controls.blackSocial + controls.blackDM;

    return static_cast<float>(player.costWhite) * sumWhite +
           static_cast<float>(player.costGrey) * sumGrey +
           static_cast<float>(player.costBlack) * sumBlack;
}

static float budgetScale(const Player& player)
{
    const float cost = computeSpendCost(player);
    if (cost <= 0.0f)
    {
        return 0.0f;
    }
    if (player.budget <= 0.0f)
    {
        return 0.0f;
    }
    if (player.budget >= cost)
    {
        return 1.0f;
    }
    return static_cast<float>(player.budget) / cost;
}

static ChannelSignal netChannelSignal(Player& A, Player& B, const BaseParameters& params)
{
    // skaluje “wydatki” jeśli budżet niewystarczający i odejmuje z budżetu
    const float sA = budgetScale(A);
    const float sB = budgetScale(B);

    // odejmij faktyczny koszt
    A.budget -= computeSpendCost(A) * sA;
    B.budget -= computeSpendCost(B) * sB;

    const auto& a = A.controls;
    const auto& b = B.controls;

    ChannelSignal sig{};

    // Broadcast: A dodatnie, B ujemne
    const float aBroad =
        channelEff(a.whiteBroadcast * sA, a.greyBroadcast * sA, a.blackBroadcast * sA);
    const float bBroad =
        channelEff(b.whiteBroadcast * sB, b.greyBroadcast * sB, b.blackBroadcast * sB);
    sig.broadcast = params.wBroadcast * (aBroad - bBroad);

    // Social (na razie tylko globalny sygnał; później dołożymy graf)
    const float aSoc = channelEff(a.whiteSocial * sA, a.greySocial * sA, a.blackSocial * sA);
    const float bSoc = channelEff(b.whiteSocial * sB, b.greySocial * sB, b.blackSocial * sB);
    sig.social       = params.wSocial * (aSoc - bSoc);

    // DM jako “dopalacz” kanału lokalnego (też może być sterowane kampanią)
    const float aDm = channelEff(a.whiteDM * sA, a.greyDM * sA, a.blackDM * sA);
    const float bDm = channelEff(b.whiteDM * sB, b.greyDM * sB, b.blackDM * sB);
    sig.dm          = params.wDM * (aDm - bDm);

    return sig;
}
void Simulation::step()
{
    m_nextGrid                       = m_currentGrid;
    const ChannelSignal globalSignal = netChannelSignal(m_playerA, m_playerB, m_parameters);

    auto idx = [&](int x, int y) -> std::size_t
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

            if (not currentCell.active)
            {
                continue;
            }

            // Presja sąsiadów
            float hDM                 = 0.0f;
            int   n                   = 0;
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
                const int s = spin(neighborCell.side);
                if (s != 0)
                {
                    hDM += static_cast<float>(s);
                    ++n;
                }
            };

            if (m_neighbourhoodType == NeighbourhoodType::VON_NEUMANN)
            {
                for (const auto& offset : Config::Neighbourhood::VN)
                {
                    accumulateNeighbour(x + static_cast<int>(offset.x()),
                                        y + static_cast<int>(offset.y()));
                }
            }
            else if (m_neighbourhoodType == NeighbourhoodType::MOORE)
            {
                for (const auto& offset : Config::Neighbourhood::MOORE)
                {
                    accumulateNeighbour(x + static_cast<int>(offset.x()),
                                        y + static_cast<int>(offset.y()));
                }
            }

            if (n > 0)
            {
                hDM /= static_cast<float>(n);
            }
            else
            {
                hDM = 0.0f;
            }
            hDM *= (m_parameters.wLocal);

            float h = 0.0f;
            h += m_parameters.wDM * hDM;
            h += globalSignal.broadcast;
            h += globalSignal.social;
            h += globalSignal.dm;

            const float theta = static_cast<float>(currentCell.threshold) * m_parameters.thetaScale;
            const float margin = m_parameters.margin;

            CellData& next = m_nextGrid[i];

            if (currentCell.side == Side::NONE)
            {
                if (h >= theta + margin)
                {
                    next.side = Side::A;
                }
                else if (h <= -(theta + margin))
                {
                    next.side = Side::B;
                }

                next.hysteresis = std::max(0.0, currentCell.hysteresis -
                                                    static_cast<double>(m_parameters.hysDecay));
            }
            else
            {
                const float thetaSwitch =
                    theta *
                    (1.0f + m_parameters.switchKappa * static_cast<float>(currentCell.hysteresis));

                if (currentCell.side == Side::A && h <= -(thetaSwitch + margin))
                {
                    next.side = Side::B;
                }
                else if (currentCell.side == Side::B && h >= thetaSwitch + margin)
                {
                    next.side = Side::A;
                }
            }
        }
    }
    m_currentGrid.swap(m_nextGrid);
    ++m_iteration;
}

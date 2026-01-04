#include "Model.hpp"
#include "Types.hpp"

#include <cstdint>

struct CampaignDiag
{
        float plannedCostA = 0, plannedCostB = 0;
        float scaleA = 0, scaleB = 0;
        float spentA = 0, spentB = 0;

        float effA_broadcast = 0, effB_broadcast = 0;
        float effA_social = 0, effB_social = 0;
        float effA_dm = 0, effB_dm = 0;

        float ctrlA_broadcast_sum = 0, ctrlB_broadcast_sum = 0;
        float ctrlA_social_sum = 0, ctrlB_social_sum = 0;
        float ctrlA_dm_sum = 0, ctrlB_dm_sum = 0;

        float stockA = 0, stockB = 0; // m_broadcastStockA/B

        void logFinancials(float costA, float costB, float scA, float scB)
        {
            plannedCostA = costA;
            plannedCostB = costB;
            scaleA       = scA;
            scaleB       = scB;
            spentA       = costA * scA;
            spentB       = costB * scB;
        }

        void logStock(float stA, float stB)
        {
            stockA = stA;
            stockB = stB;
        }
};

struct StepTransitions
{
        int N_to_A = 0, N_to_B = 0;
        int A_to_NONE = 0, B_to_NONE = 0;
        int A_to_B = 0, B_to_A = 0;

        void record(Side from, Side to)
        {
            if (from == to)
            {
                return;
            }

            if (from == Side::NONE)
            {
                if (to == Side::A)
                {
                    ++N_to_A;
                }
                else if (to == Side::B)
                {
                    ++N_to_B;
                }
            }
            else if (from == Side::A and to == Side::NONE)
            {
                ++A_to_NONE;
            }
            else if (from == Side::B and to == Side::NONE)
            {
                ++B_to_NONE;
            }
        }
};

struct FlipTracker
{
        Side    pendingForm = Side::NONE;
        uint8_t age         = 0; // ile kroków siedzi w NONE od ostatniej zmiany
};

struct StepStats
{
        int    iter   = 0;
        int    active = 0, countA = 0, countB = 0, countN = 0;
        double shareA = 0, shareB = 0, shareN = 0;

        double avgHysA = 0, avgHysB = 0;

        double internalSumHysA = 0;
        double internalSumHysB = 0;

        CampaignDiag    campaign;
        StepTransitions trans;
        GlobalSignals   gSignals;

        float          budgetA = 0.0f, budgetB = 0.0f; // stan PO wydatku w danym kroku
        BaseParameters paramsSnapshot{};               // wartości suwaków użyte w tym kroku

        void addCell(const CellData& cell)
        {
            if (not cell.active)
            {
                return;
            }

            ++active;
            switch (cell.side)
            {
            case Side::A:
                ++countA;
                internalSumHysA += cell.hysteresis;
                break;
            case Side::B:
                ++countB;
                internalSumHysB += cell.hysteresis;
                break;
            case Side::NONE:
                ++countN;
                break;
            }
        }

        void finalize()
        {
            if (active > 0)
            {
                shareA = static_cast<double>(countA) / active;
                shareB = static_cast<double>(countB) / active;
                shareN = static_cast<double>(countN) / active;
            }
            else
            {
                shareA = shareB = shareN = 0.0;
            }

            avgHysA = (countA > 0) ? internalSumHysA / countA : 0.0;
            avgHysB = (countB > 0) ? internalSumHysB / countB : 0.0;
        }
};

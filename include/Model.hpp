#pragma once

struct BaseParameters
{
        float broadcastDecay         = 0.02f; // 2% wygaszenia/iterację
        float broadcastNeutralWeight = 0.2f;  // jak mocno broadcast wpływa neutralnych (perswazja)
        float broadcastHysGain  = 0.02f; // jak mocno broadcast wzmacnia histerezę zwolenników
        float broadcastStockMax = 1.0f;

        // TODO: zrobić suwaki do 6 poniższych
        float openMindDM     = 0.55f; // [0..1]
        float openMindSocial = 0.60f; // [0..1]

        // Histereza z DM / Social: wzmacnianie vs erozja (skalowane |sygnałem|)
        float dmHysGain      = 0.02f;  // ~[0..0.10]
        float dmHysErode     = 0.04f;  // ~[0..0.10]
        float socialHysGain  = 0.015f; // ~[0..0.10]
        float socialHysErode = 0.03f;  // ~[0..0.10]

        float wBroadcast = 0.0f, wSocial = 0.0f,
              wDM = 0.0f; // wagi ile "warte" są poszczególne kanały (Broadcast, Social, DM) w
                          // obliczaniu ekspozycji, jak bardzo społeczeństwo ufa danemu medium
        float wLocal =
            0.0f; // waga lokalnych sąsiadów (ile "warci" są sąsiedzi w danym sąsiedztwie)

        float thetaScale = 0.2f; // Próg decyzji: ile trzeba "pola" żeby neutralny wszedł w A/B
        float margin     = 0.0f; // Margines przewagi (żeby remisy nie powodowały fluktuacji)

        // Histereza (utrudnia zmianę A<->B)
        float switchKappa = 0.5f;  // κ: jak histereza podbija próg zmiany strony
        float hysDecay    = 0.01f; // jak szybko zanika bez wsparcia
        float hysMaxTotal = 2.0f;  // clamp, żeby histereza nie urosła w kosmos
};

struct Controls // dokładają sygnał do kanału na korzyść strony gracza w danym kroku
{
        float whiteBroadcast = 0.0f, whiteSocial = 0.0f, whiteDM = 0.0f;
        float greyBroadcast = 0.0f, greySocial = 0.0f, greyDM = 0.0f;
        float blackBroadcast = 0.0f, blackSocial = 0.0f, blackDM = 0.0f;
        // clang-format off
        [[nodiscard]] float sumBroadcast() const { return whiteBroadcast + greyBroadcast + blackBroadcast; }
        [[nodiscard]] float sumSocial() const { return whiteSocial + greySocial + blackSocial; }
        [[nodiscard]] float sumDM() const { return whiteDM + greyDM + blackDM; }
        // clang-format on
};

struct Player
{
        Controls controls; // "pokrętła" jak mocno dana strona używa danego kanału/koloru propagandy
                           // w danym kroku, inaczej intensywność sygnału
        float budget    = 1000.0f; // budżet jaki gracz może przeznaczyć na kampanię
        float costWhite = 1.0f, costGrey = 1.2f, costBlack = 1.5f;

        [[nodiscard]] float calculatePlannedCost() const
        {
            return static_cast<float>(costWhite) *
                       (controls.whiteBroadcast + controls.whiteSocial + controls.whiteDM) +
                   static_cast<float>(costGrey) *
                       (controls.greyBroadcast + controls.greySocial + controls.greyDM) +
                   static_cast<float>(costBlack) *
                       (controls.blackBroadcast + controls.blackSocial + controls.blackDM);
        }
};

struct GlobalSignals
{
        float broadcastA = 0.0f;
        float broadcastB = 0.0f;

        float socialPressure = 0.0f;
        float dmPressure     = 0.0f;

        [[nodiscard]] float broadcastBias() const { return broadcastA - broadcastB; }
};

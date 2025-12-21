#pragma once
#include "Types.hpp"

// Broadcast to jest "TV/internet/sondaże", globalny szum, który dodaje ekspozycję każdej komórce w
// zależności od tego, ile A/B inwestuje w *Broadcast

// Social to byłby dodatkowy, bardziej skomplikowany kanał losowych połączeń między komórkami
// (niekoniecznie sąsiadujących w siatce)

// DM to "lokalne rozmowy / prywatne wiadomości", które w CA są po prostu wpływem sąsiadów w
// siatce (NeighbourhoodType, wLocal).

struct BaseParameters
{
        float wBroadcast = 0.0f, wSocial = 0.0f,
              wDM = 0.0f; // wagi ile "warte" są poszczególne kanały (Broadcast, Social, DM) w
                          // obliczaniu ekspozycji, jak bardzo społeczeństwo ufa danemu medium
        float wLocal =
            1.0f; // waga lokalnych sąsiadów (ile "warci" są sąsiedzi w danym sąsiedztwie)

        float thetaScale = 1.0f; // Próg decyzji: ile trzeba "pola" żeby neutralny wszedł w A/B
        float margin     = 0.0f; // Margines przewagi (żeby remisy nie powodowały fluktuacji)

        // Histereza (utrudnia zmianę A<->B)
        float switchKappa = 1.0f;  // κ: jak histereza podbija próg zmiany strony
        float hysGrow     = 0.02f; // jak szybko rośnie histereza przy zgodnych bodźcach
        float hysDecay    = 0.01f; // jak szybko zanika bez wsparcia
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
        float  budget    = 0.0f; // budżet jaki gracz może przeznaczyć na kampanię
        double costWhite = 1.0, costGrey = 2.0, costBlack = 3.0;

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

struct GlobalSignals // przeliczone sygnały "kto krzyczy głośniej" + dla A, - dla B
{
        float broadcastPressure = 0.0f;
        float socialPressure    = 0.0f;
        float dmPressure        = 0.0f;

        [[nodiscard]] float sum() const { return broadcastPressure + socialPressure + dmPressure; }
};

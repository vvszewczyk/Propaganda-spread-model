#pragma once
#include "Types.hpp"

// Broadcast to jest "TV/internet/sondaże", globalny szum, który dodaje ekspozycję każdej komórce w
// zależności od tego, ile A/B inwestuje w *Broadcast

// Social to byłby dodatkowy, bardziej skomplikowany kanał losowych połączeń między komórkami
// (niekoniecznie sąsiadujących w siatce)

// DM to "lokalne rozmowy / prywatne wiadomości", które w CA są po prostu wpływem sąsiadów w
// siatce (NeighbourhoodType, wLocal).

// β₀ (beta0) – jak łatwo komórka „łapie” przekaz
// → bazowa „czułość” na ekspozycję:  przy przejściu S → E albo S/E → I

// ρ₀ (rho0) – jak łatwo z „Exposed” przejść do „Infested”
// → bazowe E → I (przyjęcie narracji po zauważeniu)

// γ₀ (gamma0) – jak szybko komórka przestaje szerzyć przekaz
// → I → R (uodpornienie, fact-check)

// δ₀ (delta0) – jak szybko wypada z gry
// → I → D (ban/zmęczenie)

struct BaseParameters
{
        float beta0 = 0.0f, gamma0 = 0.0f, rho0 = 0.0f,
              delta0     = 0.0f; // współczynniki tego jak łatwo przejść między stanami
        float sBroadcast = 0.0f, sSocial = 0.0f,
              sDM = 0.0f; // wagi ile "warte" są poszczególne kanały (Broadcast, Social, DM) w
                          // obliczaniu ekspozycji, jak bardzo społeczeństwo ufa danemu medium
        float wLocal =
            0.0f; // waga lokalnych sąsiadów (ile "warci" są sąsiedzi w danym sąsiedztwie)
};

struct Controls
{
        float whiteBroadcast = 0.0f, whiteSocial = 0.0f, whiteDM = 0.0f;
        float greyBroadcast = 0.0f, greySocial = 0.0f, greyDM = 0.0f;
        float blackBroadcast = 0.0f, blackSocial = 0.0f, blackDM = 0.0f;
};

struct Player
{
        Controls controls; // "pokrętła" jak mocno dana strona używa danego kanału/koloru propagandy
                           // w danym kroku, inaczej intensywność sygnału
        float  budget    = 0.0f; // budżet jaki gracz może przeznaczyć na kampanię
        double costWhite = 1.0, costGrey = 2.0, costBlack = 3.0; // ceny jednostek danej propagandy
};

struct Pools
{
        float deltaNational = 0.0f;         // różnica poparcia
        float pBand = 0.0f, etaBand = 0.0f; // parametry bandwagon effect - wszyscy idą za liderem
        float pUnder   = 0.0f,
              etaUnder = 0.0f;  // parametry underdog effect - wszyscy kibicują przegrywającemu
        bool perState  = false; // czy liczymy globalnie czy na stan
};

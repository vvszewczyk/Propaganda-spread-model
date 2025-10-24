#pragma once
#include "Types.hpp"

struct BaseParameters
{
        float beta0 = 0.0f, gamma0 = 0.0f, rho0 = 0.0f, delta0 = 0.0f;
        float sBroadcast = 0.0f, sSocial = 0.0f, sDM = 0.0f;
        float wLocal = 0.0f;
};

struct Controls
{
        float whiteBroadcast = 0.0f, whiteSocial = 0.0f, whiteDM = 0.0f;
        float greyBroadcast = 0.0f, greySocial = 0.0f, greyDM = 0.0f;
        float blackBroadcast = 0.0f, blackSocial = 0.0f, blackDM = 0.0f;
};

struct Player
{
        Controls controls;
        float    budget    = 0.0f;
        double   costWhite = 1.0, costGrey = 2.0, costBlack = 3.0;
};

struct Pools
{
        float deltaNational = 0.0f;
        float pBand = 0.0f, etaBand = 0.0f;
        float pUnder = 0.0f, etaUnder = 0.0f;
        bool  perState = false;
};

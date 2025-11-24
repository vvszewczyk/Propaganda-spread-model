#pragma once

#include "Types.hpp"

#include <QColor>
#include <array>

namespace app
{
    struct Palette
    {
            static constexpr QColor Suspectible() noexcept { return QColor(255, 255, 255); }
            static constexpr QColor Exposed() noexcept { return QColor(255, 200, 0); }
            static constexpr QColor Infected() noexcept { return QColor(255, 0, 0); }
            static constexpr QColor Recovered() noexcept { return QColor(0, 200, 0); }
            static constexpr QColor Deceased() noexcept { return QColor(100, 100, 100); }
            static QColor           forState(State s) noexcept
            {
                switch (s)
                {
                case State::S:
                    return Suspectible();
                case State::E:
                    return Exposed();
                case State::I:
                    return Infected();
                case State::R:
                    return Recovered();
                case State::D:
                    return Deceased();
                default:
                    return Qt::black;
                }
            }
    };
} // namespace app

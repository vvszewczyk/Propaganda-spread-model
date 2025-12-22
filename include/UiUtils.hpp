#pragma once
#include <QObject>
#include <QWidget>
#include <type_traits>
#include <utility>

namespace app::ui
{
    template <typename T, typename Parent, typename Fn = std::nullptr_t, typename... Args>
    T* makeWidget(Parent* parent, Fn fn, Args&&... ctorArgs)
    {
        static_assert(std::is_base_of_v<QObject, T>, "T must derive from QObject / QWidget");
        auto* widget = new T(std::forward<Args>(ctorArgs)..., parent);

        if constexpr (std::is_invocable_v<Fn, T*>)
        {
            std::forward<Fn>(fn)(widget);
        }

        return widget;
    }
} // namespace app::ui

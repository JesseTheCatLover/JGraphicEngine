//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <functional>
#include <utility>
#include <type_traits>

template<typename... Args>
class TDelegate
{
public:
    using FCallback = std::function<void(Args...)>;

    TDelegate() = default;
    TDelegate(const TDelegate&) = default;
    TDelegate& operator=(const TDelegate&) = default;
    TDelegate(TDelegate&&) noexcept = default;
    TDelegate& operator=(TDelegate&&) noexcept = default;

    void Bind(FCallback cb)
    {
        m_Callback = std::move(cb);
    }

    void Unbind()
    {
        m_Callback = nullptr;
    }

    [[nodiscard]] bool IsBound() const
    {
        return static_cast<bool>(m_Callback);
    }

    // Prefer in most runtime code: safe no-op if unbound.
    void ExecuteIfBound(Args... args) const
    {
        if (m_Callback) m_Callback(std::forward<Args>(args)...);
    }

    // Prefer in code paths where being unbound is a bug.
    // If we don't have asserts, keep ExecuteIfBound only.
    void Execute(Args... args) const
    {
        // TODO: Replace with future project's assert macro.
        // e.g. JASSERT(IsBound());
        if (!m_Callback)
            throw std::bad_function_call();

        m_Callback(std::forward<Args>(args)...);
    }

    void operator()(Args... args) const
    {
        Execute(std::forward<Args>(args)...);
    }

private:
    FCallback m_Callback;
};

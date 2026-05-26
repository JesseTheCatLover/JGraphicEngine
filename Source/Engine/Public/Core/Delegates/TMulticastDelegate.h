//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

#include "Core/Delegates/FDelegateHandle.h"

template<typename... Args>
class TMulticastDelegate
{
public:
    using FCallback = std::function<void(Args...)>;

private:
    struct FEntry
    {
        FDelegateHandle handle;
        FCallback callback;
    };

    std::vector<FEntry> m_Listeners;
    std::size_t m_NextID = 1;

public:
    TMulticastDelegate() = default;

    // Returns a handle you can store to remove later.
    [[nodiscard]] FDelegateHandle Add(FCallback cb)
    {
        FDelegateHandle h{ m_NextID++ };
        m_Listeners.push_back(FEntry{ h, std::move(cb) });
        return h;
    }

    // Convenience alias of Add(FCallback cb)
    [[nodiscard]] FDelegateHandle AddLambda(FCallback cb) { return Add(std::move(cb)); }

    bool Remove(FDelegateHandle handle)
    {
        if (!handle.IsValid()) return false;

        const auto before = m_Listeners.size();
        m_Listeners.erase(
            std::remove_if(m_Listeners.begin(), m_Listeners.end(),
                [&](const FEntry& e) { return e.handle == handle; }),
            m_Listeners.end());
        return m_Listeners.size() != before;
    }

    void Clear()
    {
        m_Listeners.clear();
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return m_Listeners.empty();
    }

    void Broadcast(Args... args)
    {
        // Snapshot iteration to remain well-defined if listeners mutate the delegate during broadcast.
        // This trades allocations for correctness/simplicity.
        auto snapshot = m_Listeners;

        for (auto& e : snapshot)
        {
            if (e.callback)
                e.callback(args...);
        }
    }
};

//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <vector>

#include "Core/Delegates/TMulticastDelegate.h"
#include "Scene/FSelectionModifiers.h"

struct FSelectionModifiers;

template<typename T>
class TSelectionModel
{
public:
    using FOnChanged = TMulticastDelegate<>;

private:
    std::vector<T> m_Selected;
    T m_Anchor{};
    T m_RevealRequest{};

    FOnChanged m_OnChanged;

public:
    [[nodiscard]] const std::vector<T>& GetSelection() const { return m_Selected; }
    [[nodiscard]] const T& GetAnchor() const { return m_Anchor; }

    // ---- Events ----
    [[nodiscard]] FOnChanged& OnChanged() { return m_OnChanged; }
    [[nodiscard]] const FOnChanged& OnChanged() const { return m_OnChanged; }

    [[nodiscard]] bool IsSelected(const T& value) const
    {
        return std::find(m_Selected.begin(), m_Selected.end(), value) != m_Selected.end();
    }

    [[nodiscard]] bool IsSelectionEmpty() const
    {
        return m_Selected.empty();
    }

    // ---- Commands ----
    bool ApplyClick(const T& value, const FSelectionModifiers& mods, const std::vector<T>* visibleOrder)
    {
        if (IsNullValue(value))
            return Clear();

        bool changed = false;

        if (mods.bRange && visibleOrder)
            changed = SelectRangeTo(value, *visibleOrder);
        else if (mods.bToggle)
            changed = Toggle(value);
        else
            changed = SelectSingle(value);

        if (changed)
            NotifyChanged();

        return changed;
    }

    T ConsumeRevealRequest()
    {
        T out = m_RevealRequest;
        m_RevealRequest = T{};
        return out;
    }

    bool Clear()
    {
        if (m_Selected.empty() && IsNullValue(m_Anchor) && IsNullValue(m_RevealRequest))
            return false;

        m_Selected.clear();
        m_Anchor = T{};
        m_RevealRequest = T{};

        NotifyChanged();
        return true;
    }

private:
    bool SelectSingle(const T& value)
    {
        const bool sameSelection =
            m_Selected.size() == 1 &&
            m_Selected.front() == value &&
            m_Anchor == value &&
            m_RevealRequest == value;

        if (sameSelection)
            return false;

        m_Selected.clear();
        if (!IsNullValue(value))
            m_Selected.push_back(value);

        m_Anchor = value;
        m_RevealRequest = value;
        return true;
    }

    bool Toggle(const T& value)
    {
        auto it = std::find(m_Selected.begin(), m_Selected.end(), value);
        if (it == m_Selected.end())
            m_Selected.push_back(value);
        else
            m_Selected.erase(it);

        m_Anchor = value;
        m_RevealRequest = value;
        return true;
    }

    bool SelectRangeTo(const T& value, const std::vector<T>& order)
    {
        if (IsNullValue(m_Anchor))
            return SelectSingle(value);

        auto itA = std::find(order.begin(), order.end(), m_Anchor);
        auto itB = std::find(order.begin(), order.end(), value);
        if (itA == order.end() || itB == order.end())
            return SelectSingle(value);

        auto begin = std::min(itA, itB);
        auto end   = std::max(itA, itB);

        std::vector<T> newSelection;
        newSelection.reserve(static_cast<std::size_t>((end - begin) + 1));

        for (auto it = begin; it != end + 1; ++it)
            newSelection.push_back(*it);

        const bool changed = (m_Selected != newSelection) || (m_RevealRequest != value);

        m_Selected = std::move(newSelection);
        m_RevealRequest = value;
        return changed;
    }

    void NotifyChanged()
    {
        m_OnChanged.Broadcast();
    }

    static bool IsNullValue(const T& value)
    {
        return value == T{};
    }
};
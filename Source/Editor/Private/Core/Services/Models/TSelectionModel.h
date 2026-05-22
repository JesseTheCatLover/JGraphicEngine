//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <vector>

#include "Scene/FSelectionModifiers.h"

struct FSelectionModifiers;

template<typename T>
class TSelectionModel
{
public:
    using FListener = std::function<void()>;
    using FListenerID = std::size_t;

private:
    std::vector<T> m_Selected;
    T m_Anchor{};
    T m_RevealRequest{};

    struct FListenerEntry
    {
        FListenerID id = 0;
        FListener callback;
    };

    std::vector<FListenerEntry> m_Listeners;
    FListenerID m_NextListenerID = 1;

public:
    [[nodiscard]] const std::vector<T>& GetSelection() const { return m_Selected; }
    [[nodiscard]] const T& GetAnchor() const { return m_Anchor; }

    FListenerID AddChangedListener(FListener listener)
    {
        const FListenerID id = m_NextListenerID++;
        m_Listeners.push_back({ id, std::move(listener) });
        return id;
    }

    void RemoveChangedListener(FListenerID id)
    {
        m_Listeners.erase(
            std::remove_if(m_Listeners.begin(), m_Listeners.end(),
                [id](const FListenerEntry& e) { return e.id == id; }),
            m_Listeners.end());
    }

    [[nodiscard]] bool IsSelected(const T& value) const
    {
        return std::find(m_Selected.begin(), m_Selected.end(), value) != m_Selected.end();
    }

    [[nodiscard]] bool IsSelectionEmpty() const
    {
        return m_Selected.empty();
    }

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
        auto end = std::max(itA, itB);

        std::vector<T> newSelection;
        for (auto it = begin; it != end + 1; ++it)
            newSelection.push_back(*it);

        const bool changed = (m_Selected != newSelection) || (m_RevealRequest != value);

        m_Selected = std::move(newSelection);
        m_RevealRequest = value;
        return changed;
    }

    void NotifyChanged()
    {
        for (const FListenerEntry& listener : m_Listeners)
        {
            if (listener.callback)
                listener.callback();
        }
    }

    static bool IsNullValue(const T& value)
    {
        return value == T{};
    }
};
//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <vector>

#include "Core/Delegates/TMulticastDelegate.h"
#include "EditorAPI/Scene/FSelectionModifiers.h"

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
    // ---- Query ----
    [[nodiscard]] const std::vector<T>& GetSelection() const { return m_Selected; }
    [[nodiscard]] const T& GetAnchor() const { return m_Anchor; }

    [[nodiscard]] bool IsSelected(const T& value) const
    {
        return std::find(m_Selected.begin(), m_Selected.end(), value) != m_Selected.end();
    }

    [[nodiscard]] bool IsSelectionEmpty() const
    {
        return m_Selected.empty();
    }

    // ---- Events ----
    [[nodiscard]] FOnChanged& OnChanged() { return m_OnChanged; }
    [[nodiscard]] const FOnChanged& OnChanged() const { return m_OnChanged; }

    // ---- Click-driven interaction ----
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

    // ---- Programmatic commands ----
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

    bool SelectSingle(const T& value)
    {
        if (IsNullValue(value))
            return Clear();

        const bool sameSelection =
            m_Selected.size() == 1 &&
            m_Selected.front() == value &&
            m_Anchor == value &&
            m_RevealRequest == value;

        if (sameSelection)
            return false;

        m_Selected.clear();
        m_Selected.push_back(value);
        m_Anchor = value;
        m_RevealRequest = value;

        NotifyChanged();
        return true;
    }

    bool Select(const T& value)
    {
        if (IsNullValue(value))
            return false;

        const bool alreadySelected = IsSelected(value);
        const bool anchorChanged = (m_Anchor != value);
        const bool revealChanged = (m_RevealRequest != value);

        if (!alreadySelected)
            m_Selected.push_back(value);

        m_Anchor = value;
        m_RevealRequest = value;

        const bool changed = !alreadySelected || anchorChanged || revealChanged;
        if (changed)
            NotifyChanged();

        return changed;
    }

    bool Deselect(const T& value)
    {
        if (IsNullValue(value))
            return false;

        auto it = std::find(m_Selected.begin(), m_Selected.end(), value);
        if (it == m_Selected.end())
            return false;

        const bool wasAnchor = (m_Anchor == value);
        const bool wasReveal = (m_RevealRequest == value);

        m_Selected.erase(it);

        if (wasAnchor)
        {
            if (!m_Selected.empty())
                m_Anchor = m_Selected.back();
            else
                m_Anchor = T{};
        }

        if (wasReveal)
            m_RevealRequest = T{};

        NotifyChanged();
        return true;
    }

    bool Toggle(const T& value)
    {
        if (IsNullValue(value))
            return false;

        auto it = std::find(m_Selected.begin(), m_Selected.end(), value);
        if (it == m_Selected.end())
        {
            m_Selected.push_back(value);
            m_Anchor = value;
            m_RevealRequest = value;
        }
        else
        {
            const bool wasAnchor = (m_Anchor == value);
            const bool wasReveal = (m_RevealRequest == value);

            m_Selected.erase(it);

            if (wasAnchor)
                m_Anchor = m_Selected.empty() ? T{} : m_Selected.back();

            if (wasReveal)
                m_RevealRequest = T{};
        }

        NotifyChanged();
        return true;
    }

    bool SetSelection(std::vector<T> values)
    {
        NormalizeSelection(values);

        T newAnchor = values.empty() ? T{} : values.back();
        T newReveal = newAnchor;

        if (m_Selected == values && m_Anchor == newAnchor && m_RevealRequest == newReveal)
            return false;

        m_Selected = std::move(values);
        m_Anchor = newAnchor;
        m_RevealRequest = newReveal;

        NotifyChanged();
        return true;
    }

    bool SetSelection(std::initializer_list<T> values)
    {
        return SetSelection(std::vector<T>(values));
    }

    bool SelectRangeTo(const T& value, const std::vector<T>& order)
    {
        if (IsNullValue(value))
            return false;

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
        {
            if (!IsNullValue(*it))
                newSelection.push_back(*it);
        }

        NormalizeSelection(newSelection);

        const bool changed =
            (m_Selected != newSelection) ||
            (m_RevealRequest != value);

        m_Selected = std::move(newSelection);
        m_RevealRequest = value;

        if (!m_Selected.empty() && IsNullValue(m_Anchor))
            m_Anchor = m_Selected.front();

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

private:
    void NotifyChanged()
    {
        m_OnChanged.Broadcast();
    }

    static bool IsNullValue(const T& value)
    {
        return value == T{};
    }

    static void NormalizeSelection(std::vector<T>& values)
    {
        values.erase(
            std::remove_if(values.begin(), values.end(),
                [](const T& value) { return IsNullValue(value); }),
            values.end());

        std::vector<T> unique;
        unique.reserve(values.size());

        for (const T& value : values)
        {
            if (std::find(unique.begin(), unique.end(), value) == unique.end())
                unique.push_back(value);
        }

        values = std::move(unique);
    }
};

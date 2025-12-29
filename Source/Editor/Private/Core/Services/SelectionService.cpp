//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "SelectionService.h"

#include "EditorRuntime.h"
#include "Scene/FSelectionModifiers.h"

SelectionService::SelectionService(EditorRuntime &rt)
: m_Runtime(rt)
{}

void SelectionService::ApplyClick(ActorID id, const FSelectionModifiers &mods, const std::vector<ActorID> *visibleOrder)
{
    if (id == 0) { Clear(); return; }

    if (mods.bRange && visibleOrder)
        SelectRangeTo(id, *visibleOrder);
    else if (mods.bToggle)
        Toggle(id);
    else
        SelectSingle(id);
}

SelectionService::ActorID SelectionService::ConsumeRevealRequest()
{
    const ActorID out = m_RevealRequest;
    m_RevealRequest = 0;
    return out;
}

void SelectionService::PushToRuntime()
{
    m_Runtime.GetScene().SetSelectedActors(m_Selected);
}

void SelectionService::SelectSingle(ActorID id)
{
    m_Selected.clear();
    if (id != 0) m_Selected.push_back(id);

    m_Anchor = id;
    m_RevealRequest = id;

    PushToRuntime();
}

void SelectionService::Toggle(ActorID id)
{
    auto it = std::find(m_Selected.begin(), m_Selected.end(), id);
    if (it == m_Selected.end()) m_Selected.push_back(id);
    else m_Selected.erase(it);
    m_Anchor = id;
    m_RevealRequest = id;
    PushToRuntime();
}

void SelectionService::SelectRangeTo(ActorID id, const std::vector<ActorID> &order)
{
    if (m_Anchor == 0) { SelectSingle(id); return; }

    auto itA = std::find(order.begin(), order.end(), m_Anchor);
    auto itB = std::find(order.begin(), order.end(), id);
    if (itA == order.end() || itB == order.end()) { SelectSingle(id); return; }

    auto begin = std::min(itA, itB);
    auto end   = std::max(itA, itB);

    m_Selected.clear();
    for (auto it = begin; it != end + 1; ++it)
        m_Selected.push_back(*it);

    m_RevealRequest = id;
    PushToRuntime(); // (OS explorer behavior)
}

void SelectionService::Clear()
{
    m_Selected.clear();
    m_Anchor = 0;
    m_RevealRequest = 0;
    PushToRuntime();
}
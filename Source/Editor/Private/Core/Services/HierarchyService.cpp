//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "HierarchyService.h"

#include <functional>

#include "SceneQueryService.h"
#include "SelectionService.h"
#include "Core/EditorHost.h"
#include "UI/Panels/SceneHierarchyPanel.h"

static void BuildVisibleOrderRecursive(
    ActorID parent,
    const std::vector<FHierarchySnapshot>& snap,
    std::vector<ActorID>& outOrder)
{
    for (const auto& n : snap)
    {
        if (n.parentID == parent)
        {
            outOrder.push_back(n.id);
            if (n.hasChildren)
                BuildVisibleOrderRecursive(n.id, snap, outOrder);
        }
    }
}

HierarchyService::HierarchyService(EditorHost &host)
: m_Host(host)
{}

void HierarchyService::Tick(float)
{
    if (!m_Dirty) return;

    auto& queries = m_Host.GetService<SceneQueryService>();
    m_Snapshot = queries.BuildHierarchySnapshot();

    const auto& selected = m_Host.GetService<SelectionService>().GetSelection();
    for (auto& node : m_Snapshot)
    {
        node.isSelected =
            std::find(selected.begin(), selected.end(), node.id) != selected.end();
    }

    RebuildVisibleOrder();
    m_Dirty = false;
}

void HierarchyService::RebuildVisibleOrder()
{
    m_VisibleOrder.clear();
    m_VisibleOrder.reserve(m_Snapshot.size());
    BuildVisibleOrderRecursive(0, m_Snapshot, m_VisibleOrder);
}
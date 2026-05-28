//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "HierarchyService.h"

#include <functional>

#include "SceneQueryService.h"
#include "Selection/SelectionService.h"
#include "Controllers/Outputs/FHierarchyOutput.h"
#include "Core/EditorHost.h"

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

    RebuildVisibleOrder();
    m_Dirty = false;
}

void HierarchyService::RebuildVisibleOrder()
{
    m_VisibleOrder.clear();
    m_VisibleOrder.reserve(m_Snapshot.size());
    BuildVisibleOrderRecursive(0, m_Snapshot, m_VisibleOrder);
}
//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorCore/Services/SceneHierarchyService.h"

#include <functional>

#include "EditorCore/Services/SceneQueryService.h"
#include "Panels/Controllers/Outputs/FHierarchyOutput.h"
#include "EditorCore/EditorHost.h"

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
            if (n.bHasChildren)
                BuildVisibleOrderRecursive(n.id, snap, outOrder);
        }
    }
}

SceneHierarchyService::SceneHierarchyService(EditorHost &host, EditorRuntime& runtime)
    : m_Host(host)
    , m_Runtime(runtime)
{}

void SceneHierarchyService::Tick(float)
{
    if (!m_Dirty) return;

    auto& queries = m_Host.GetService<SceneQueryService>();
    m_Snapshot = queries.BuildHierarchySnapshot();

    RebuildVisibleOrder();
    m_Dirty = false;
}

void SceneHierarchyService::ReparentActor(ActorID childID, ActorID newParentID)
{
    if (m_Runtime.GetScene().ReparentActor(childID, newParentID))
    {
        MarkDirty(); // Rebuild snapshot on next Tick
    }
}

void SceneHierarchyService::DestroyActor(ActorID actorID)
{
    m_Runtime.GetScene().DeleteActors({ actorID });
    MarkDirty();
}

void SceneHierarchyService::RenameActor(ActorID actorID, const std::string& newName)
{
    m_Runtime.GetScene().SetActorName(actorID, newName);
    MarkDirty();
}

void SceneHierarchyService::ToggleVisibility(ActorID actorID)
{
    m_Runtime.GetScene().ToggleActorVisibility(actorID);
    MarkDirty();
}

void SceneHierarchyService::RebuildVisibleOrder()
{
    m_VisibleOrder.clear();
    m_VisibleOrder.reserve(m_Snapshot.size());
    BuildVisibleOrderRecursive(0, m_Snapshot, m_VisibleOrder);
}

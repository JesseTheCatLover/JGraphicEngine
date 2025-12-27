//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "PickingService.h"

#include "HierarchyService.h"
#include "SceneQueryService.h"
#include "SelectionService.h"
#include "Core/EditorHost.h"
#include "Scene/FSelectionModifiers.h"


PickingService::PickingService(EditorHost &host)
: m_Host(host)
{}

uint64_t PickingService::PickActor(const CameraEditorTool &cam, float width, float height, float x, float y)
{
    if (width <= 0.f || height <= 0.f) return 0;

    FRay ray = BuildRay(cam, width, height, x, y);

    FRaycastHit hit{};
    auto& queries = m_Host.GetService<SceneQueryService>();
    if (queries.Raycast(ray, hit) && hit.bHit)
        return hit.actorID;

    return 0;
}

void PickingService::ApplyPickSelection(uint64_t actorId, const FSelectionModifiers &mods)
{
    auto& sel = m_Host.GetService<SelectionService>();
    const std::vector<uint64_t>* order = nullptr;

    if (mods.bRange)
        order = &m_Host.GetService<HierarchyService>().GetVisibleOrder();

    sel.ApplyClick(actorId, mods, order);
}
FRay PickingService::BuildRay(const CameraEditorTool &cam, float width, float height, float x, float y)
{
    FRay out{};
    // ...
    return out;
}

//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "ScenePickingService.h"

#include "SceneQueryService.h"
#include "Core/EditorHost.h"
#include "Core/Math/FViewportMath.h"

ScenePickingService::ScenePickingService(EditorHost &host)
: m_Host(host)
{}

ActorID ScenePickingService::PickActorAtViewportPos(const CameraEditorTool& cam, float w, float h, float x, float y) const
{
    if (w <= 0.f || h <= 0.f) return 0;

    const FRay ray = FViewportMath::BuildRayFromCamera(cam, w, h, x, y);

    FRaycastHit hit{};
    auto& queries = m_Host.GetService<SceneQueryService>();
    if (queries.RaycastIntoTheScene(ray, hit) && hit.bHit)
        return hit.actorID;

    return 0;
}
//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "SceneQueryService.h"

#include "EditorRuntime.h"

SceneQueryService::SceneQueryService(EditorRuntime &runtime)
: m_Runtime(runtime)
{}

bool SceneQueryService::RaycastIntoTheScene(const FRay &ray, FRaycastHit &outHit)
{
    return m_Runtime.GetScene().RaycastIntoTheScene(ray, outHit);
}

bool SceneQueryService::TryGetActorWorldTransform(ActorID id, FTransform &outXf) const
{
    return m_Runtime.GetScene().TryGetActorWorldTransform(id, outXf);
}

std::vector<FHierarchySnapshot> SceneQueryService::BuildHierarchySnapshot()
{
    return m_Runtime.GetScene().BuildHierarchySnapshot();
}

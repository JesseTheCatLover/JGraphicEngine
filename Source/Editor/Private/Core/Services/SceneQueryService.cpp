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

bool SceneQueryService::TrySetActorWorldTransform(ActorID id, const FTransform &xf)
{
    return m_Runtime.GetScene().TrySetActorWorldTransform(id, xf);
}

bool SceneQueryService::TrySetActorWorldLocation(ActorID id, const FVector3 &p)
{
    return m_Runtime.GetScene().TrySetActorWorldLocation(id, p);
}

bool SceneQueryService::TrySetActorWorldRotation(ActorID id, const FQuat &q)
{
    return m_Runtime.GetScene().TrySetActorWorldRotation(id, q);
}

bool SceneQueryService::TrySetActorWorldScale(ActorID id, const FVector3 &s)
{
    return m_Runtime.GetScene().TrySetActorWorldScale(id, s);
}

//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "SceneQueryService.h"

#include "EditorRuntime.h"

SceneQueryService::SceneQueryService(EditorRuntime &runtime)
: m_Runtime(runtime)
{}

bool SceneQueryService::Raycast(const FRay &ray, FRaycastHit &outHit)
{
    return m_Runtime.GetScene().Raycast(ray, outHit);
}

std::vector<FHierarchySnapshot> SceneQueryService::BuildHierarchySnapshot()
{
    return m_Runtime.GetScene().BuildHierarchySnapshot();
}

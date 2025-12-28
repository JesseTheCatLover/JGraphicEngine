//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>

#include "Core/IEditorService.h"
#include "Scene/FHierarchySnapshot.h"

class EditorRuntime;
struct FRaycastHit;
struct FRay;

class SceneQueryService : public IEditorService
{
private:
    EditorRuntime& m_Runtime;

public:
    explicit SceneQueryService(EditorRuntime& runtime);

    bool Raycast(const FRay& ray, FRaycastHit& outHit);

    std::vector<FHierarchySnapshot> BuildHierarchySnapshot();
};

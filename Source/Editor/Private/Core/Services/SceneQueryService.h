//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/IEditorService.h"

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

    auto BuildHierarchySnapshot();
};

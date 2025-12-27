//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <cstdint>

#include "FHierarchySnapshot.h"
#include "FRaycast.h"

class JScene;
class SceneManager;
class EngineContext;

class EditorSceneAPI
{
private:
    using ActorID = uint64_t;

    EngineContext& m_Context;
    SceneManager& m_SceneManager;

    std::vector<ActorID> m_SelectedActors;

public:
    EditorSceneAPI(EngineContext& ctx, SceneManager& scene);

    JScene* GetActiveScene();

    [[nodiscard]] std::vector<FHierarchySnapshot>
        BuildHierarchySnapshot() const;
    void SetSelectedActors(const std::vector<ActorID>& ids);
    void DeleteActors(const std::vector<ActorID>& ids);
    void DuplicateActors(const std::vector<ActorID>& ids);

    bool Raycast(const FRay& ray, FRaycastHit& outHit);
};

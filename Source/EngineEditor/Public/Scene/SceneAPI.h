//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>

#include "FEditorActoSnapshot.h"

class SceneManager;
class EngineContext;

class EditorSceneAPI
{
private:
    EngineContext& m_Context;
    SceneManager& m_SceneManager;

    std::vector<ActorID> m_SelectedActors;

public:
    EditorSceneAPI(EngineContext& ctx, SceneManager& scene);

    [[nodiscard]] std::vector<FEditorActorSnapshot>
        BuildHierarchySnapshot() const;
    void SetSelectedActors(const std::vector<ActorID>& ids);
    void DeleteActors(const std::vector<ActorID>& ids);
    void DuplicateActors(const std::vector<ActorID>& ids);
};

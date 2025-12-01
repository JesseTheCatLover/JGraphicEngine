//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>

#include "FEditorActoSnapshot.h"

class SceneManager;
class EngineContext;

class EditorSceneAPI
{
    friend class EditorCore;
private:
    EngineContext& m_Context;
    SceneManager& m_SceneManager;

    std::vector<ActorID> m_SelectedActors;

public:
    EditorSceneAPI(EngineContext& ctx, SceneManager& scene);

    // ---- Read-only for panels ----
    [[nodiscard]] std::vector<FEditorActorSnapshot>
        BuildHierarchySnapshot() const;

private:
    // ---- Mutating ops (EditorCore only) ----
    void SetSelectedActors(const std::vector<ActorID>& ids);
    void DeleteActors(const std::vector<ActorID>& ids);
    void DuplicateActors(const std::vector<ActorID>& ids);
};

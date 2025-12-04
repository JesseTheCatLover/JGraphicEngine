//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/SceneAPI.h"

#include "Framework/SceneManager.h"
#include "Scene/JScene.h"

EditorSceneAPI::EditorSceneAPI(EngineContext &ctx, SceneManager &scene):
m_Context(ctx),
m_SceneManager(scene)
{
}

std::vector<FEditorActorSnapshot> EditorSceneAPI::BuildHierarchySnapshot() const
{
    std::vector<FEditorActorSnapshot> result;

    JScene* scene = m_SceneManager.GetActiveScene();
    if (!scene)
        return result;

    const auto& actors = m_SceneManager.ListAllActors();
    result.reserve(actors.size());

    for (auto* actor : actors)
    {
        if (!actor) continue;

        FEditorActorSnapshot info{};
        info.id = actor->GetRuntimeID();
        info.parentID = actor->GetParentActor()
                            ? actor->GetParentActor()->GetRuntimeID()
                            : 0;
        info.name = actor->GetName();
        info.hasChildren = !actor->GetChildActors().empty();
        info.isSelected  = false; // will be filled by EditorCore

        result.push_back(std::move(info));
    }

    return result;
}

void EditorSceneAPI::SetSelectedActors(const std::vector<ActorID> &ids)
{
    m_SelectedActors = ids;
    // Later: propagate to renderer for outlines, etc.
}

void EditorSceneAPI::DeleteActors(const std::vector<ActorID> &ids)
{
    for (ActorID id : ids)
        m_SceneManager.ImmediateDestroyActor(id);
}

void EditorSceneAPI::DuplicateActors(const std::vector<ActorID> &ids)
{
    // TODO: implement cloning later
}

//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <cstdint>

#include "EditorCore/IEditorService.h"
#include "EditorAPI/Scene/FHierarchySnapshot.h"

class EditorRuntime;
class EditorHost;

using ActorID = uint64_t;

class SceneHierarchyService : public IEditorService
{
private:
    EditorHost& m_Host;
    EditorRuntime& m_Runtime;

    bool m_Dirty = true;
    std::vector<FHierarchySnapshot> m_Snapshot;
    std::vector<uint64_t> m_VisibleOrder;

    void RebuildVisibleOrder();

public:
    explicit SceneHierarchyService(EditorHost& host, EditorRuntime& runtime);

    void Tick(float deltaTime) override;

    void MarkDirty() { m_Dirty = true; }

    [[nodiscard]] const std::vector<FHierarchySnapshot>& GetSnapshot() const { return m_Snapshot; }
    [[nodiscard]] const std::vector<ActorID>& GetVisibleOrder() const { return m_VisibleOrder; }

    void ReparentActor(ActorID childID, ActorID newParentID);
    void DestroyActor(ActorID actorID);
    void RenameActor(ActorID actorID, const std::string& newName);
    void ToggleVisibility(ActorID actorID);
};

//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.
#pragma once
#include <unordered_set>
#include <vector>
#include <string>
#include <cstdint>

#include "UI/IEditorPanels.h"
#include "Controllers/Inputs/FHierarchyPanelInput.h"

class SelectionService;
using ActorID = uint64_t;
struct FHierarchyOutput;
struct FHierarchySnapshot;

class SceneHierarchyPanel : public IEditorPanel
{
    std::string m_PanelKey = "Hierarchy:0";
    bool m_bClickedAnyItemThisFrame = false;

    // UI-only persistent state
    std::unordered_set<ActorID> m_OpenNodes;
    ActorID m_ScrollTo = 0;

    void ApplyRevealRequest(const std::vector<FHierarchySnapshot>& actors, ActorID reveal);
    void DrawActorNode(const FHierarchySnapshot& node,
                       const std::vector<FHierarchySnapshot>& allActors,
                       FHierarchyPanelInput& ioInput,
                       const SelectionService& selection);

public:
    [[nodiscard]] const char* GetName() const override { return "Scene Hierarchy"; }
    [[nodiscard]] const char* GetPanelKey() const override { return m_PanelKey.c_str(); }

    void OnDestroy(EditorHost& host) override;
    void Draw(EditorHost& host) override;
};
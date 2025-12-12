//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include "UI/IEditorPanels.h"

struct FEditorActorSnapshot;
class EditorContext;

class SceneHierarchyPanel : public IEditorPanel
{
private:
    bool bClickedAnyItemThisFrame = false;

    void DrawActorNode(
    const FEditorActorSnapshot& node,
    const std::vector<FEditorActorSnapshot>& allActors,
    EditorCore& core);

public:
    const char* GetName() const override { return "Scene Hierarchy"; }

    void Draw(EditorContext& context, EditorCore& core) override;
};
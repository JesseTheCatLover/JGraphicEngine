//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "SceneHierarchyPanel.h"

#include "imgui.h"
#include "../EditorContext.h"
#include "Core/EngineState.h"
#include "Scene/JActor.h"

void SceneHierarchyPanel::Draw(const EditorContext& context)
{
    const auto &actors = context.GetState().GetSceneActors();
    ImGui::Begin("Scene Hierarchy");
    for (const auto &actor : actors)
    {
        ImGui::Text("%s", actor.Name.c_str());
    }
    ImGui::End();
}

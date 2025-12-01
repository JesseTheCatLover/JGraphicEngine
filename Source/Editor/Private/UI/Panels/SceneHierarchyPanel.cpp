//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "SceneHierarchyPanel.h"
#include "EditorContext.h"
#include "Core/EditorCore.h"
#include "imgui.h"

// You will replace these with your real scene/actor APIs
struct Actor
{
    int id;
    const char* name;
};

static std::vector<Actor> DebugGetActorsFromEngine(EditorContext& ctx)
{
    // your scene/actor system. This is just a placeholder.
    (void)ctx;
    return {
            { 0, "Camera" },
            { 1, "Light" },
            { 2, "Player" }
    };
}

void SceneHierarchyPanel::Draw(EditorContext& context, EditorCore& core)
{
    if (!ImGui::Begin(GetName()))
    {
        ImGui::End();
        return;
    }

    auto actors = DebugGetActorsFromEngine(context);

    for (const auto& actor : actors)
    {

    }

    ImGui::End();
}

//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "SceneHierarchyPanel.h"
#include "EditorContext.h"
#include "Core/EditorCore.h"
#include "imgui.h"
#include "Core/FSelectionModifiers.h"

void SceneHierarchyPanel::DrawActorNode(
    const FEditorActorSnapshot& node,
    const std::vector<FEditorActorSnapshot>& allActors,
    EditorCore& core)
{
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (!node.hasChildren)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (node.isSelected)
        flags |= ImGuiTreeNodeFlags_Selected;

    const bool opened = ImGui::TreeNodeEx(
        (void*)(intptr_t)node.id,
        flags,
        "%s", node.name.c_str()
    );

    // Click selection (ignore clicks that are only toggling open)
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        bClickedAnyItemThisFrame = true;

        ImGuiIO& io = ImGui::GetIO();
        FSelectionModifiers mods;
        mods.bRange = io.KeyShift;
        mods.bToggle = io.KeyCtrl || io.KeySuper; // Ctrl on Win/Linux, Cmd on mac

        core.HandleSelectionClick(node.id, mods);
    }

    if (opened && node.hasChildren)
    {
        for (const auto& child : allActors)
        {
            if (child.parentID == node.id)
                DrawActorNode(child, allActors, core);
        }
        ImGui::TreePop();
    }
}

void SceneHierarchyPanel::Draw(EditorContext& context, EditorCore& core)
{
    if (!ImGui::Begin(GetName()))
    {
        ImGui::End();
        return;
    }
    // Later we add toolbar buttons here (add actor, search, filters, etc.)
    const auto& actors = core.GetHierarchySnapshot();
    if (actors.empty())
    {
        //ImGui::TextUnformatted("No actors in scene.");
        ImGui::End();
        return;
    }

    bClickedAnyItemThisFrame = false;

    for (const auto& actor : actors)
        if (actor.parentID == 0)
            DrawActorNode(actor, actors, core);

    // Background click clears selection
    if (ImGui::IsWindowHovered() &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !bClickedAnyItemThisFrame)
    {
        core.ClearSelection();
    }

    ImGui::End();
}
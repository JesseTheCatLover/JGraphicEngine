//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "SceneHierarchyPanel.h"

#include "imgui.h"
#include "Core/EditorHost.h"
#include "Subsystems/SceneHierarchySubsystem.h"
#include "Controllers/Outputs/FHierarchyOutput.h"
#include "Core/Services/Selection/SelectionService.h"

// helpers
static bool FindNodeById(const std::vector<FHierarchySnapshot>& actors, ActorID id, FHierarchySnapshot& outNode)
{
    for (const auto& n : actors)
        if (n.id == id) { outNode = n; return true; }
    return false;
}

static void BuildParentChain(const std::vector<FHierarchySnapshot>& actors, ActorID leaf, std::vector<ActorID>& outChain)
{
    ActorID cur = leaf;
    while (cur != 0)
    {
        outChain.push_back(cur);
        FHierarchySnapshot node{};
        if (!FindNodeById(actors, cur, node)) break;
        cur = node.parentID;
    }
}

void SceneHierarchyPanel::ApplyRevealRequest(const std::vector<FHierarchySnapshot>& actors, ActorID reveal)
{
    if (reveal == 0) return;

    std::vector<ActorID> chain;
    chain.reserve(8);
    BuildParentChain(actors, reveal, chain);

    for (ActorID id : chain)
        m_OpenNodes.insert(id);

    m_ScrollTo = reveal;
}

void SceneHierarchyPanel::DrawActorNode(const FHierarchySnapshot& node,
                                        const std::vector<FHierarchySnapshot>& allActors,
                                        FHierarchyPanelInput& ioInput,
                                        const TSelectionModel<ActorID>& selection)
{
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (!node.hasChildren)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (selection.IsSelected(node.id))
        flags |= ImGuiTreeNodeFlags_Selected;

    const bool wantOpen = (m_OpenNodes.count(node.id) > 0);
    ImGui::SetNextItemOpen(wantOpen, ImGuiCond_Always);

    const bool opened = ImGui::TreeNodeEx((void*)(intptr_t)node.id, flags, "%s", node.name.c_str());

    if (ImGui::IsItemToggledOpen())
    {
        if (opened) m_OpenNodes.insert(node.id);
        else        m_OpenNodes.erase(node.id);
    }

    if (m_ScrollTo == node.id)
    {
        ImGui::SetScrollHereY(0.25f);
        m_ScrollTo = 0;
    }

    // click selection (ignore clicks that just toggled open)
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen())
    {
        m_bClickedAnyItemThisFrame = true;
        ioInput.bClickedItem = true;
        ioInput.clickedActor = node.id;
    }

    if (opened && node.hasChildren)
    {
        for (const auto& child : allActors)
            if (child.parentID == node.id)
                DrawActorNode(child, allActors, ioInput, selection);

        ImGui::TreePop();
    }
}

void SceneHierarchyPanel::OnDestroy(EditorHost& host)
{
}

void SceneHierarchyPanel::Draw(EditorHost& host)
{
    if (!ImGui::Begin(GetName()))
    {
        ImGui::End();
        return;
    }

    // Build input for this frame
    FHierarchyPanelInput input{};
    input.panelKey = GetPanelKey();

    input.bFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    input.bHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    ImGuiIO& io = ImGui::GetIO();
    input.bCtrl  = io.KeyCtrl;
    input.bShift = io.KeyShift;
    input.bSuper = io.KeySuper;

    m_bClickedAnyItemThisFrame = false;

    // Output snapshot
    const FHierarchyOutput* out = host.GetSubsystem<SceneHierarchySubsystem>().GetOutput(GetPanelKey());
    if (!out || !out->bHasSnapshot || !out->snapshot)
    {
        ImGui::TextUnformatted("Hierarchy: waiting for snapshot...");
        // IMPORTANT: still submit input so controller gets created next tick
        host.GetSubsystem<SceneHierarchySubsystem>().SubmitInput(input);
        ImGui::End();
        return;
    }

    const auto& actors = *out->snapshot;

    // Apply reveal request
    ApplyRevealRequest(actors, out->revealActorID);

    const auto& sceneSelection = host.GetService<SelectionService>().GetSceneActorSelection();

    // Draw tree roots
    for (const auto& actor : actors)
        if (actor.parentID == 0)
            DrawActorNode(actor, actors, input, sceneSelection);

    // Background click clears selection
    if (ImGui::IsWindowHovered() &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !m_bClickedAnyItemThisFrame)
    {
        input.bClearSelection = true;
    }

    // Submit input last
    host.GetSubsystem<SceneHierarchySubsystem>().SubmitInput(input);

    ImGui::End();
}
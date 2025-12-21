//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "SceneHierarchyPanel.h"
#include "EditorContext.h"
#include "Core/EditorHost.h"
#include "imgui.h"
#include "Core/FSelectionModifiers.h"

static bool FindNodeById(
    const std::vector<FEditorActorSnapshot>& actors,
    ActorID id,
    FEditorActorSnapshot& outNode)
{
    for (const auto& n : actors)
    {
        if (n.id == id)
        {
            outNode = n;
            return true;
        }
    }
    return false;
}

static void BuildParentChain(const std::vector<FEditorActorSnapshot>& actors, ActorID leaf,
    std::vector<ActorID>& outChain)
{
    // leaf -> parent -> ... until root
    ActorID cur = leaf;
    while (cur != 0)
    {
        outChain.push_back(cur);

        FEditorActorSnapshot node{};
        if (!FindNodeById(actors, cur, node))
            break;

        cur = node.parentID;
    }
}

void SceneHierarchyPanel::ApplyRevealRequest(const std::vector<FEditorActorSnapshot>& actors, EditorHost& core)
{
    const ActorID reveal = core.ConsumeRevealRequest();
    if (reveal == 0)
        return;

    std::vector<ActorID> chain;
    chain.reserve(8);
    BuildParentChain(actors, reveal, chain);

    // Expand all nodes in the chain so the leaf becomes visible.
    for (ActorID id : chain)
        m_OpenNodes.insert(id);

    // Scroll the target into view when we draw it.
    m_ScrollTo = reveal;
}

void SceneHierarchyPanel::DrawActorNode(
    const FEditorActorSnapshot& node,
    const std::vector<FEditorActorSnapshot>& allActors,
    EditorHost& core)
{
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (!node.hasChildren)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (node.isSelected)
        flags |= ImGuiTreeNodeFlags_Selected;

    // Apply persistent open-state to ImGui.
    const bool bWantOpen = (m_OpenNodes.count(node.id) > 0);
    ImGui::SetNextItemOpen(bWantOpen, ImGuiCond_Always);

    const bool bOpened = ImGui::TreeNodeEx(
        (void*)(intptr_t)node.id,
        flags,
        "%s", node.name.c_str()
    );

    // If user toggled this node open/closed, sync back to the set
    if (ImGui::IsItemToggledOpen())
    {
        if (bOpened) m_OpenNodes.insert(node.id);
        else m_OpenNodes.erase(node.id);
    }

    // Scroll into view if this is the reveal target
    if (m_ScrollTo == node.id)
    {
        ImGui::SetScrollHereY(0.25f);
        m_ScrollTo = 0;
    }

    // Click selection (ignore clicks that are only toggling open)
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        bClickedAnyItemThisFrame = true;

        ImGuiIO& io = ImGui::GetIO();
        FSelectionModifiers mods;
        mods.bRange  = io.KeyShift;
        mods.bToggle = io.KeyCtrl || io.KeySuper;

        core.HandleSelectionClick(node.id, mods);
    }

    if (bOpened && node.hasChildren)
    {
        for (const auto& child : allActors)
        {
            if (child.parentID == node.id)
                DrawActorNode(child, allActors, core);
        }
        ImGui::TreePop();
    }
}

void SceneHierarchyPanel::Draw(EditorContext& context, EditorHost& core)
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

    // First: if viewport selection requested reveal, expand + queue scroll.
    ApplyRevealRequest(actors, core); // TODO: Make this optional in setting for future with something like bSyncHierarchyToSelection

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

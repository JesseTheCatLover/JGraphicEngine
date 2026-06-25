//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Panels/Controllers/SceneHierarchyController.h"

#include "EditorCore/EditorHost.h"
#include "EditorCore/Services/SceneHierarchyService.h"
#include "EditorCore/Services/Selection/SelectionService.h"
#include "Panels/Controllers/Inputs/FHierarchyPanelInput.h"
#include "Panels/Controllers/Outputs/FHierarchyOutput.h"
#include "EditorAPI/Scene/FSelectionModifiers.h"

SceneHierarchyController::SceneHierarchyController(PanelID id, EditorHost &host)
: m_PanelID(id), m_Host(host)
{}

void SceneHierarchyController::Update(float deltaTime, const FHierarchyPanelInput& input, FHierarchyOutput& out)
{
    auto& hierarchy = m_Host.GetService<SceneHierarchyService>();
    auto& selection = m_Host.GetService<SelectionService>().GetSceneActorSelection();

    const auto& snap = hierarchy.GetSnapshot();
    out.snapshot = &snap;
    out.bHasSnapshot = !snap.empty();

    // Click item -> selection
    if (input.bClickedItem && input.clickedActor != 0 && out.bHasSnapshot)
    {
        FSelectionModifiers mods{};
        mods.bRange  = input.bShift;
        mods.bToggle = input.bCtrl || input.bSuper;

        selection.ApplyClick(input.clickedActor, mods, mods.bRange ? &hierarchy.GetVisibleOrder() : nullptr);
    }

    // Background click clears selection
    if (input.bClearSelection)
    {
        selection.Clear();
    }

    // Reveal (viewport selection -> hierarchy expand/scroll)
    out.revealActorID = selection.ConsumeRevealRequest();

    // Reparenting
    if (input.bReparentRequested && input.draggedActor != input.targetParentActor) // TODO: Implement Undo History On These Actions
    {
        hierarchy.ReparentActor(input.draggedActor, input.targetParentActor);
    }

    // Deletion
    if (input.bDeleteRequested)
    {
        hierarchy.DestroyActor(input.targetActorToModify);
    }

    // Renaming
    if (input.bRenameRequested)
    {
        hierarchy.RenameActor(input.targetActorToModify, input.newName);
    }

    // Visibility
    if (input.bToggleVisibilityRequested)
    {
        hierarchy.ToggleVisibility(input.targetActorToModify);
    }
}

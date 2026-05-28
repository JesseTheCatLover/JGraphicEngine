//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Controllers/SceneHierarchyController.h"

#include "Core/EditorHost.h"
#include "Core/Services/HierarchyService.h"
#include "Core/Services/Selection/SelectionService.h"
#include "Controllers/Inputs/FHierarchyPanelInput.h"
#include "Controllers/Outputs/FHierarchyOutput.h"
#include "Scene/FSelectionModifiers.h"

SceneHierarchyController::SceneHierarchyController(PanelID id, EditorHost &host)
: m_PanelID(id), m_Host(host)
{}

void SceneHierarchyController::Update(float deltaTime, const FHierarchyPanelInput& input, FHierarchyOutput& out)
{
    auto& hierarchy = m_Host.GetService<HierarchyService>();
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
}

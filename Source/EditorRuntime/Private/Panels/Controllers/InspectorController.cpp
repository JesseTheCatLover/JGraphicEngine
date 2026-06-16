//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Panels/Controllers/InspectorController.h"
#include "Panels/Controllers/Documents/ActorInspectorProvider.h"

#include "EditorCore/EditorHost.h"
#include "EditorCore/Services/Selection/SelectionService.h"

#include "Panels/Controllers/Inputs/FInspectorPanelInput.h"
#include "Panels/Controllers/Outputs/FInspectorOutput.h"

InspectorController::InspectorController(PanelID id, EditorHost& host)
    : m_PanelID(id)
    , m_Host(host)
{}

InspectorController::~InspectorController()
{

}

void InspectorController::Update(float /*deltaTime*/, const FInspectorPanelInput& input, FInspectorOutput& out)
{
    auto& selection = m_Host.GetService<SelectionService>();

    out = {};
    const auto& sceneSelection = m_Host.GetService<SelectionService>().GetSceneActorSelection();
    const auto& selected = sceneSelection.GetSelection();


    if (selected.empty())
    {
        out.statusText = "Inspector: nothing selected.";
        return;
    }
    if (selected.size() > 1) // TODO: Implement multi-selection for actors for the inspector panel
    {
        out.statusText = "Inspector: multi-select not supported yet.";
        return;
    }

    const uint64_t runtimeID = selected[0];
    out.bHasSelection = true;
    out.selectedActor = runtimeID;

    if (!m_ActorProvider)
        m_ActorProvider = MakeUnique<ActorInspectorProvider>(m_Host);

    FInspectorSelection sel;
    sel.runtimeID = runtimeID;

    // 1) apply edits first
    for (const auto& cmd : input.edits)
        m_ActorProvider->ApplyEdit(cmd);

    // 2) rebuild
    m_Doc.targets.clear();
    m_ActorProvider->BuildDocument(sel, m_Doc);

    out.document = &m_Doc;
    out.bHasDocument = true;

    if (m_Doc.targets.empty())
        out.statusText = "Inspector: no reflected properties visible.";
}
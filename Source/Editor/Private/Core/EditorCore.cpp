//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "EditorCore.h"
#include "EditorContext.h"
#include <algorithm>

#include "Core/JEngine.h"
#include "Rendering/IPlatformSurface.h"

EditorCore::EditorCore(EditorContext &context, EngineEditor& engineEditor):
m_Context(context),
m_EngineEditor(engineEditor)
{
}

void EditorCore::Tick(float deltaTime)
{
    PushFrameInfoToEditorContext();

    TickEditorTools(deltaTime);

    SubmitEditorViewSources();

    ClearFrameStates();

    // // Pull a fresh snapshot of the scene hierarchy
    // m_HierarchySnapshot = m_EngineEditor.GetScene().BuildHierarchySnapshot();

    // // Mark which actors are selected in the snapshot
    // UpdateSelectionFlagsInHierarchy();
    //
    // // Push selection state down to EngineEditor for rendering overlays, gizmos, etc.
    // SyncSelectionToEngine();
    //
    // // Later: tools, camera update, background jobs, etc.

}

void EditorCore::PushFrameInfoToEditorContext()
{
    // Pull frame info to EditorContext
    m_FrameSnapshot = m_EngineEditor.GetViewport().GetFrameSnapshot();
    m_Context.SetFrameSnapshot(m_FrameSnapshot);
}

void EditorCore::TickEditorTools(float deltaTime)
{
    // Build tool frame states
    FEditorToolFrameState toolState;
    // Determine which camera ID is active, if any
    if (m_ActiveViewportPanel)
    {
        auto it = m_PanelToCameraMap.find(m_ActiveViewportPanel);
        if (it != m_PanelToCameraMap.end())
            toolState.activeCameraId = it->second;
    }
    toolState.cameraAspectMap = m_CameraAspectMap;

    // Let EngineEditor tick all tools
    m_EngineEditor.TickAllTools(deltaTime, toolState);
    m_EngineEditor.SubmitEditorViewSources(toolState);
}

void EditorCore::SubmitEditorViewSources()
{
    //m_EngineEditor.SubmitEditorViewSources();
}

void EditorCore::ClearFrameStates()
{
    m_CameraAspectMap.clear();
}

// ---------------- Selection ----------------
//
// void EditorCore::SelectSingle(ActorID id)
// {
//     std::vector<ActorID> sel;
//     if (id != 0)
//         sel.push_back(id);
//
//     m_Context.SetSelection(sel);
//     SyncSelectionToEngine();
// }
//
// void EditorCore::AddToSelection(ActorID id)
// {
//     if (id == 0)
//         return;
//
//     auto selection = m_Context.GetSelection();
//     if (std::find(selection.begin(), selection.end(), id) == selection.end())
//     {
//         selection.push_back(id);
//         m_Context.SetSelection(selection);
//         SyncSelectionToEngine();
//     }
// }
//
// void EditorCore::ToggleSelection(ActorID id)
// {
//     if (id == 0)
//         return;
//
//     auto selection = m_Context.GetSelection();
//     auto it = std::find(selection.begin(), selection.end(), id);
//
//     if (it != selection.end())
//     {
//         // remove it
//         selection.erase(it);
//     }
//     else
//     {
//         selection.push_back(id);
//     }
//
//     m_Context.SetSelection(selection);
//     SyncSelectionToEngine();
// }
//
// void EditorCore::ClearSelection()
// {
//     std::vector<ActorID> empty;
//     m_Context.SetSelection(empty);
//     SyncSelectionToEngine();
// }
//
// const std::vector<ActorID>& EditorCore::GetSelection() const
// {
//     return m_Context.GetSelection();
// }
//
// void EditorCore::SyncSelectionToEngine()
// {
//     m_EngineEditor.GetScene().SetSelectedActors(m_Context.GetSelection());
// }
//
// void EditorCore::UpdateSelectionFlagsInHierarchy()
// {
//     const auto& selection = m_Context.GetSelection();
//
//     for (auto& node : m_HierarchySnapshot)
//     {
//         node.isSelected = std::find(selection.begin(), selection.end(), node.id)
//                           != selection.end();
//     }
// }
//
//
// // ---------------- Scene operations ----------------
//
// void EditorCore::DeleteSelectedActors()
// {
//     const auto& selection = m_Context.GetSelection();
//     if (selection.empty())
//         return;
//
//     // Call into EngineEditor to actually remove actors from the scene
//     m_EngineEditor.GetScene().DeleteActors(selection);
//
//     // Clear editor selection (they're gone now)
//     ClearSelection();
//
//     // Optionally: you might want to refresh hierarchy snapshot explicitly,
//     // but next Update() will do it anyway.
// }


// ---------------- Gizmo/tools ----------------
//
// void EditorCore::SetGizmoMode(EGizmoMode mode)
// {
//     m_Context.SetGizmoMode(mode);
//     // Later: tell EngineEditor or Gizmo system about this mode if needed
// }
//
// EGizmoMode EditorCore::GetGizmoMode() const
// {
//     return m_Context.GetGizmoMode();
// }


void* EditorCore::GetViewportTextureHandle() const
{
    auto& viewport = m_EngineEditor.GetViewport();
    return viewport.GetNativeTextureHandle(viewport.GetViewportColor());
}

void EditorCore::CreateCameraForPanel(const IEditorPanel* panel)
{
    if (m_PanelToCameraMap.count(panel) > 0)
        return;

    auto id = m_EngineEditor.CreateCameraEditorTool();
    m_PanelToCameraMap[panel] = id;
}

void EditorCore::DestroyCameraForPanel(const IEditorPanel* panel)
{
    auto it = m_PanelToCameraMap.find(panel);
    if (it == m_PanelToCameraMap.end())
        return;

    UDynamicID::IDType id = it->second;

    // 1) destroy in EngineEditor
    m_EngineEditor.DestroyCameraEditorTool(id);

    // 2) remove mappings
    m_PanelToCameraMap.erase(it);
    DeactivateCameraForPanel(panel);
    m_CameraAspectMap.erase(id);
}

void EditorCore::SetViewportFocused(const IEditorPanel *panel, bool bFocused)
{
    IPlatformSurface* surface = JEngine::Get().GetPlatformSurface();
    if (bFocused)
    {
        if (surface)
            surface->SetCursorMode(ECursorMode::Disabled);

        ActivateCameraForPanel(panel);
    }
    else
    {
        if (surface)
            surface->SetCursorMode(ECursorMode::Visible);

        DeactivateCameraForPanel(panel);
    }
}

void EditorCore::ActivateCameraForPanel(const IEditorPanel *panel)
{
    m_ActiveViewportPanel = panel;
}

void EditorCore::DeactivateCameraForPanel(const IEditorPanel *panel)
{
    if (m_ActiveViewportPanel == panel)
        m_ActiveViewportPanel = nullptr;
}

void EditorCore::OnViewportResized(const IEditorPanel *panel, float aspectRatio)
{
    auto it = m_PanelToCameraMap.find(panel);
    if (it == m_PanelToCameraMap.end())
    {
        CreateCameraForPanel(panel);
        it = m_PanelToCameraMap.find(panel);
    }

    const auto id = it->second;
    m_CameraAspectMap[id] = aspectRatio;
}

void EditorCore::SetSizeTemp(float width, float height)
{
    m_EngineEditor.GetViewport().SetSceneViewportSize(width, height);
}

void EditorCore::ExecuteCommand(TUniquePtr<IEditorCommand> cmd)
{
    if (!cmd)
        return;

    cmd->Apply(m_Context);

    m_UndoStack.push(std::move(cmd));

    // Once a new command is executed, redo history is invalid.
    while (!m_RedoStack.empty())
        m_RedoStack.pop();
}

void EditorCore::Undo()
{
    if (m_UndoStack.empty())
        return;

    auto cmd = TakeUniqueOwnership(m_UndoStack.top());
    m_UndoStack.pop();

    cmd->Undo(m_Context);

    m_RedoStack.push(TakeUniqueOwnership(cmd));
}

void EditorCore::Redo()
{
    if (m_RedoStack.empty())
        return;

    auto cmd = TakeUniqueOwnership(m_RedoStack.top());
    m_RedoStack.pop();

    cmd->Apply(m_Context);

    m_UndoStack.push(TakeUniqueOwnership(cmd));
}

void EditorCore::SelectActor(int actorId)
{
    // Simple helper for now; no undo. We can later convert this into a command.
}
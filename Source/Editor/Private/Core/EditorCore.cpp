//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "EditorCore.h"
#include "EditorContext.h"
#include <algorithm>

EditorCore::EditorCore(EditorContext &context, EngineEditor& engineEditor):
m_Context(context),
m_EngineEditor(engineEditor)
{
}

void EditorCore::Update(float deltaTime)
{
    (void)deltaTime;

    // 1) Pull fresh frame info from the engine (viewport size, scene color, etc.)
    m_FrameSnapshot = m_EngineEditor.GetViewport().GetFrameSnapshot();
    m_Context.SetFrameSnapshot(m_FrameSnapshot);

    // 2) Pull a fresh snapshot of the scene hierarchy
    m_HierarchySnapshot = m_EngineEditor.GetScene().BuildHierarchySnapshot();

    // 3) Mark which actors are selected in the snapshot
    UpdateSelectionFlagsInHierarchy();

    // 4) Push selection state down to EngineEditor for rendering overlays, gizmos, etc.
    SyncSelectionToEngine();

    // Later: tools, camera update, background jobs, etc.
}

// ---------------- Selection ----------------

void EditorCore::SelectSingle(ActorID id)
{
    std::vector<ActorID> sel;
    if (id != 0)
        sel.push_back(id);

    m_Context.SetSelection(sel);
    SyncSelectionToEngine();
}

void EditorCore::AddToSelection(ActorID id)
{
    if (id == 0)
        return;

    auto selection = m_Context.GetSelection();
    if (std::find(selection.begin(), selection.end(), id) == selection.end())
    {
        selection.push_back(id);
        m_Context.SetSelection(selection);
        SyncSelectionToEngine();
    }
}

void EditorCore::ToggleSelection(ActorID id)
{
    if (id == 0)
        return;

    auto selection = m_Context.GetSelection();
    auto it = std::find(selection.begin(), selection.end(), id);

    if (it != selection.end())
    {
        // remove it
        selection.erase(it);
    }
    else
    {
        selection.push_back(id);
    }

    m_Context.SetSelection(selection);
    SyncSelectionToEngine();
}

void EditorCore::ClearSelection()
{
    std::vector<ActorID> empty;
    m_Context.SetSelection(empty);
    SyncSelectionToEngine();
}

const std::vector<ActorID>& EditorCore::GetSelection() const
{
    return m_Context.GetSelection();
}

void EditorCore::SyncSelectionToEngine()
{
    m_EngineEditor.GetScene().SetSelectedActors(m_Context.GetSelection());
}

void EditorCore::UpdateSelectionFlagsInHierarchy()
{
    const auto& selection = m_Context.GetSelection();

    for (auto& node : m_HierarchySnapshot)
    {
        node.isSelected = std::find(selection.begin(), selection.end(), node.id)
                          != selection.end();
    }
}


// ---------------- Scene operations ----------------

void EditorCore::DeleteSelectedActors()
{
    const auto& selection = m_Context.GetSelection();
    if (selection.empty())
        return;

    // Call into EngineEditor to actually remove actors from the scene
    m_EngineEditor.GetScene().DeleteActors(selection);

    // Clear editor selection (they're gone now)
    ClearSelection();

    // Optionally: you might want to refresh hierarchy snapshot explicitly,
    // but next Update() will do it anyway.
}


// ---------------- Gizmo/tools ----------------

void EditorCore::SetGizmoMode(EGizmoMode mode)
{
    m_Context.SetGizmoMode(mode);
    // Later: tell EngineEditor or Gizmo system about this mode if needed
}

EGizmoMode EditorCore::GetGizmoMode() const
{
    return m_Context.GetGizmoMode();
}


void* EditorCore::GetViewportTextureHandle() const
{
    auto& viewport = m_EngineEditor.GetViewport();
    return viewport.GetNativeTextureHandle(viewport.GetViewportColor());
}

void EditorCore::OnViewportResized(int &w, int &h) const
{
    m_EngineEditor.GetViewport().SetSceneViewportSize(w, h);
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
//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorCore/Services/EditTimelineService.h"

#include <iostream>

#include "EditorCore/Services/HierarchyService.h"
#include "EditorCore/Services/ShellCommandService.h"
#include "EditorCore/EditorHost.h"

EditTimelineService::EditTimelineService(EditorHost& host)
    : m_Host(host)
{
}

void EditTimelineService::Execute(TUniquePtr<IUndoableAction> action)
{
    if (!action)
        return;

    // Run forward
    action->Do();
    ApplyEffects(*action);

    // Record in undo stack
    m_UndoStack.push_back(std::move(action));

    // New action invalidates redo history
    m_RedoStack.clear();
}

void EditTimelineService::Undo()
{
    if (m_UndoStack.empty())
        return;

    // Pop last action
    TUniquePtr<IUndoableAction> action = std::move(m_UndoStack.back());
    m_UndoStack.pop_back();

    if (action)
    {
        action->Undo();
        ApplyEffects(*action);
        m_RedoStack.push_back(std::move(action));
    }
}

void EditTimelineService::Redo()
{
    if (m_RedoStack.empty())
        return;

    TUniquePtr<IUndoableAction> action = std::move(m_RedoStack.back());
    m_RedoStack.pop_back();

    if (action)
    {
        action->Do();
        ApplyEffects(*action);
        m_UndoStack.push_back(std::move(action));
    }
}

void EditTimelineService::Clear()
{
    m_UndoStack.clear();
    m_RedoStack.clear();
}

void EditTimelineService::Shutdown()
{
    m_UndoStack.clear();
    m_RedoStack.clear();
}

void EditTimelineService::RegisterShellCommands(ShellCommandService &shell)
{
    shell.Register("Editor.History.Undo", [this](){ Undo(); });
    shell.Register("Editor.History.Redo", [this](){ Redo(); });
    shell.Register("Editor.History.Clear", [this](){ Clear(); });
}

void EditTimelineService::ApplyEffects(const IUndoableAction &action)
{
    const EEditEffect fx = action.GetEffects();

    if (HasEffect(fx, EEditEffect::Hierarchy))
        m_Host.GetService<HierarchyService>().MarkDirty();
    //
    // if (HasEffect(fx, EEditEffect::Inspector))
    //     m_Host.GetService<InspectorService>().MarkDirty();
    //
    // if (HasEffect(fx, EEditEffect::Selection))
    //     m_Host.GetService<SelectionService>().PushToRuntime();
}

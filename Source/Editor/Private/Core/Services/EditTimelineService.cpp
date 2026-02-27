//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditTimelineService.h"

EditTimelineService::EditTimelineService(EditorHost &host):
m_Host(host)
{
}

void EditTimelineService::Execute(TUniquePtr<IUndoableAction> action)
{
    if (!action)
        return;

    action->Do();
    m_UndoStack.push_back(TakeUniqueOwnership(action));
    m_RedoStack.clear();
}

bool EditTimelineService::CanUndo() const
{
    return !m_UndoStack.empty();
}

bool EditTimelineService::CanRedo() const
{
    return !m_RedoStack.empty();
}

void EditTimelineService::Undo()
{
    if (m_UndoStack.empty())
        return;

    TUniquePtr<IUndoableAction> action = TakeUniqueOwnership(m_UndoStack.back());
    m_UndoStack.pop_back();

    action->Undo();
    m_RedoStack.push_back(TakeUniqueOwnership(action));
}

void EditTimelineService::Redo()
{
    if (m_RedoStack.empty())
        return;

    TUniquePtr<IUndoableAction> action = TakeUniqueOwnership(m_RedoStack.back());
    m_RedoStack.pop_back();

    action->Do();
    m_UndoStack.push_back(TakeUniqueOwnership(action));
}

void EditTimelineService::Clear()
{
    m_UndoStack.clear();
    m_RedoStack.clear();
}
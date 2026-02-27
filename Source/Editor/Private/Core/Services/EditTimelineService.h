//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>

#include "Core/Memory/SmartPointers.h"
#include "IUndoableAction.h"
#include "Core/IEditorService.h"

class EditorHost;

class EditTimelineService : public IEditorService
{
private:
    EditorHost& m_Host;

    std::vector<TUniquePtr<IUndoableAction>> m_UndoStack;
    std::vector<TUniquePtr<IUndoableAction>> m_RedoStack;

public:
    explicit EditTimelineService(EditorHost& host);

    void Execute(TUniquePtr<IUndoableAction> action);
    bool CanUndo() const;
    bool CanRedo() const;

    void Undo();
    void Redo();

    void Clear();
};

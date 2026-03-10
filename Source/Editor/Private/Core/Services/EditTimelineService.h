//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>

#include "Core/IEditorService.h"
#include "Core/Memory/SmartPointers.h"
#include "UndoableActions/IUndoableAction.h"

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

    [[nodiscard]] bool CanUndo() const { return !m_UndoStack.empty(); }
    [[nodiscard]] bool CanRedo() const { return !m_RedoStack.empty(); }

    void Undo();
    void Redo();

    void Clear();

    void RegisterShellCommands(ShellCommandService& shell) override;
};
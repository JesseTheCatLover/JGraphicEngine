//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

class EditorContext;

/**
 * @class IEditorCommand
 * Base class for all editor actions that should support undo/redo.
 **/
class IEditorCommand
{
public:
    virtual ~IEditorCommand() = default;

    // Apply the command (do/redo).
    virtual void Apply(EditorContext& context) = 0;

    // Undo the previously applied command.
    virtual void Undo(EditorContext& context) = 0;

    // Optional: for debugging / history UI later.
    virtual const char* GetName() const = 0;
};
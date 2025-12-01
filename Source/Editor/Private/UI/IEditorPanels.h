//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

class EditorContext;
class EditorCore;

/** @class IEditorPanel
 * @brief Base interface for all editor panels.
 */
class IEditorPanel
{
public:
    virtual ~IEditorPanel() = default;

    // Unique name of the panel, used for window titles, debugging, etc.
    virtual const char* GetName() const = 0;

    // Panels should only use EditorContext to interact with the engine/editor.
    virtual void Draw(EditorContext& context, EditorCore& core) = 0;
};
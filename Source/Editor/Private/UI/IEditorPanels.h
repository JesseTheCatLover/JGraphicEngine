//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

class EditorHost;

/** @class IEditorPanel
 * @brief Base interface for all editor panels.
 */
class IEditorPanel
{
public:
    virtual ~IEditorPanel() = default;

    // Unique name of the panel, used for window titles, debugging, etc.
    [[nodiscard]] virtual const char* GetName() const = 0;

    [[nodiscard]] virtual const char* GetPanelKey() const = 0;

    // Called when the panel is added/created
    virtual void OnCreate(EditorHost& host) {}

    // Called before the panel is removed/destroyed
    virtual void OnDestroy(EditorHost& host) {}

    // Panels should only use EditorHost to interact with the engine/editor.
    virtual void Draw(EditorHost& host) = 0;
};
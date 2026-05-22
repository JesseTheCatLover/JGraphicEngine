//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

class EditorRuntime;
class EditorHost;

/**
 * @class IEditorDialog
 * @brief Base interface for all transient editor dialogs (e.g. Import, Rename, Delete).
 */
class IEditorDialog
{
public:
    virtual ~IEditorDialog() = default;

    // Called when dialog instance is first created
    virtual void OnCreate(EditorHost& host, EditorRuntime& runtime) {}

    // Called just before dialog is destroyed
    virtual void OnDestroy(EditorHost& host, EditorRuntime& runtime) {}

    // Called when dialog is opened
    virtual void OnOpen(EditorHost& host, EditorRuntime& runtime) {}

    // Called when dialog is closed
    virtual void OnClose() {}

    // Called when the dialog is requested while already open
    virtual void OnRequestFocus(EditorHost& host, EditorRuntime& runtime) {}

    // Called every frame while dialog is open
    virtual void Draw(EditorHost& host, EditorRuntime& runtime) = 0;

    // Optional debug name
    [[nodiscard]] virtual const char* GetName() const = 0;

    // Whether this dialog is still active
    [[nodiscard]] virtual bool IsOpen() const = 0;
};

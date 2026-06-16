//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

#include "EditorCore/IEditorService.h"

class EditorHost;
class InputManager;
class IHotkeyBindingEditable;
class EditorRuntime;

class HotkeyService : public IEditorService
{
private:
    EditorHost& m_Host;
    EditorRuntime& m_EditorRuntime;
    InputManager& m_InputManager;

    // Pending command IDs triggered this frame
    std::vector<std::string> m_TriggeredCommands;

public:
    explicit HotkeyService(EditorHost& host, EditorRuntime& editorRuntime);

    // Called every editor tick
    void Tick(float deltaTime) override;

    // UI uses this for menu labels/tooltips
    [[nodiscard]] std::string GetShortcutText(const std::string& commandID) const;

    // EditorHost consumes and executes these through ShellCommandService
    std::vector<std::string> ConsumeTriggeredCommands();
};

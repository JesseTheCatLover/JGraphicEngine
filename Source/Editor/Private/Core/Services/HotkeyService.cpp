//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "HotkeyService.h"

#include "EditorRuntime.h"
#include "ShellCommandService.h"
#include "Core/EditorHost.h"
#include "Framework/InputManager.h"
#include "InputSystem/MappingStyles/HotkeyChord/IHotkeyBindingEditable.h"

HotkeyService::HotkeyService(EditorHost& host, EditorRuntime& editorRuntime)
    : m_Host(host)
    , m_EditorRuntime(editorRuntime)
    , m_InputManager(m_EditorRuntime.GetSurface().GetInputManager())
{
}

void HotkeyService::Tick(float /*deltaTime*/)
{
    IHotkeyBindingEditable* bindings = m_InputManager.GetHotkeyBindings();
    if (!bindings)
        return;

    std::vector<std::string> triggered = bindings->ConsumeTriggeredCommands();
    if (triggered.empty())
        return;

    auto& shell = m_Host.GetService<ShellCommandService>();
    for (const std::string& cmd : triggered)
    {
        shell.Execute(cmd);
    }
}

std::string HotkeyService::GetShortcutText(const std::string& commandID) const
{
    const IHotkeyBindingEditable* bindings = m_InputManager.GetHotkeyBindings();
    if (!bindings)
        return {};

    return bindings->GetCommandDisplayString(commandID);
}

std::vector<std::string> HotkeyService::ConsumeTriggeredCommands()
{
    std::vector<std::string> out = std::move(m_TriggeredCommands);
    m_TriggeredCommands.clear();
    return out;
}
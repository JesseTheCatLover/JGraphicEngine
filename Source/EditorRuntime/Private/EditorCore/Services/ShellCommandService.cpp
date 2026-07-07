//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorCore/Services/ShellCommandService.h"

#include <iostream>

#include "EditorCore/EditorHost.h"
#include "EditorCore/Services/EditorFocusService.h"

ShellCommandService::ShellCommandService(EditorHost& host)
    : m_Host(host)
    , m_EditorFocus(host.GetService<EditorFocusService>())
{
}

void ShellCommandService::Register(const std::string& contextID, const std::string& commandID, FCommandHandler handler)
{
    if (commandID.empty())
    {
        std::cerr << "[ShellCommandService]: Cannot register empty command id\n";
        return;
    }

    if (!handler)
    {
        std::cerr << "[ShellCommandService]: Invalid registration for command '" << commandID << "'\n";
        return;
    }
    m_ContextHandlers[contextID][commandID] = std::move(handler);
}

void ShellCommandService::Register(const std::string &commandID, FCommandHandler handler)
{
    Register("Global", commandID, std::move(handler));
}

bool ShellCommandService::Execute(const std::string& commandID)
{
    const std::string& currentContext = m_EditorFocus.GetActiveContext();

    // 1. Try active panel context (e.g., AssetBrowser specific Ctrl+C)
    if (TryExecuteInContext(currentContext, commandID))
    {
        return true;
    }

    // 2. Bubble up to Global context (e.g., File->Save)
    if (currentContext != "Global" && TryExecuteInContext("Global", commandID))
    {
        return true;
    }
    return false;
}

bool ShellCommandService::Has(const std::string& commandID, const std::string& context) const
{
    // If context is empty, explicitly default to checking the "Global" scope
    const std::string& targetContext = context.empty() ? "Global" : context;

    // 1. Check the target context (either the requested one or Global)
    auto ctxIt = m_ContextHandlers.find(targetContext);
    if (ctxIt != m_ContextHandlers.end())
    {
        if (ctxIt->second.find(commandID) != ctxIt->second.end())
        {
            return true;
        }
    }

    // 2. If a specific panel context was requested but didn't have it, bubble up to check Global
    if (targetContext != "Global")
    {
        auto globalIt = m_ContextHandlers.find("Global");
        if (globalIt != m_ContextHandlers.end())
        {
            return globalIt->second.find(commandID) != globalIt->second.end();
        }
    }

    return false;
}

bool ShellCommandService::TryExecuteInContext(const std::string &context, const std::string &cmd)
{
    {
        auto ctxIt = m_ContextHandlers.find(context);
        if (ctxIt != m_ContextHandlers.end())
        {
            auto cmdIt = ctxIt->second.find(cmd);
            if (cmdIt != ctxIt->second.end())
            {
                cmdIt->second(); // Execute!
                return true;
            }
        }
        return false;
    }
}

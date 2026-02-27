//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "ShellCommandService.h"

#include <iostream>

ShellCommandService::ShellCommandService(EditorHost& host)
    : m_Host(host)
{
}

void ShellCommandService::Register(const std::string& commandID, FCommandHandler handler)
{
    if (commandID.empty())
    {
        std::cerr << "[ShellCommandService]: Cannot register empty command id\n";
        return;
    }

    if (!handler)
    {
        std::cerr << "[ShellCommandService]: Cannot register command '" << commandID
                  << "' with null handler\n";
        return;
    }

    m_Handlers[commandID] = std::move(handler);
}

bool ShellCommandService::Execute(const std::string& commandID)
{
    auto it = m_Handlers.find(commandID);
    if (it == m_Handlers.end())
    {
        std::cerr << "[ShellCommandService]: Unknown command '" << commandID << "'\n";
        return false;
    }

    it->second();
    return true;
}

bool ShellCommandService::Has(const std::string& commandId) const
{
    return m_Handlers.find(commandId) != m_Handlers.end();
}
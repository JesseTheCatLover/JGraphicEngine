//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <functional>
#include <string>
#include <unordered_map>

#include "../IEditorService.h"

class EditorHost;

class ShellCommandService : public IEditorService
{
public:
    using FCommandHandler = std::function<void()>;

private:
    EditorHost& m_Host;
    std::unordered_map<std::string, FCommandHandler> m_Handlers;

public:
    explicit ShellCommandService(EditorHost& host);

    void Register(const std::string& commandID, FCommandHandler handler);
    bool Execute(const std::string& commandID);

    [[nodiscard]] bool Has(const std::string& commandId) const;
};
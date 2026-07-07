//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <functional>
#include <string>
#include <unordered_map>

#include "EditorCore/IEditorService.h"

class EditorFocusService;
class EditorHost;

class ShellCommandService : public IEditorService
{
public:
    using FCommandHandler = std::function<void()>;

private:
    EditorHost& m_Host;
    EditorFocusService& m_EditorFocus;
    std::unordered_map<std::string, std::unordered_map<std::string, FCommandHandler>> m_ContextHandlers;

public:
    explicit ShellCommandService(EditorHost& host);

    // Context-aware registration
    void Register(const std::string& contextID, const std::string& commandID, FCommandHandler handler);

    // Automatically registers to "Global"
    void Register(const std::string& commandID, FCommandHandler handler);

    bool Execute(const std::string& commandID);

    [[nodiscard]] bool Has(const std::string& commandID, const std::string& context = "") const;

private:
    bool TryExecuteInContext(const std::string& context, const std::string& cmd);
};
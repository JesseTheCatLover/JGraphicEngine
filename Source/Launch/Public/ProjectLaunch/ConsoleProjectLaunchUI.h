// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "ProjectLaunch/IProjectLaunchUI.h"

class ConsoleProjectLaunchUI : public IProjectLaunchUI
{
public:
    bool PromptForProjectFile(std::string& outProjectFilePath) override;
    bool PromptForEnginePath(const std::string& projectFilePath, std::string& outEnginePath) override;
    void ShowError(const std::string& title, const std::string& message) override;
};
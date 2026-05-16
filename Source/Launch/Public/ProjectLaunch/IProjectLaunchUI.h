// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

#include "EProjectLaunchAction.h"
#include "Core/Project/FProjectCreateRequest.h"

class IProjectLaunchUI
{
public:
    virtual ~IProjectLaunchUI() = default;
    
    virtual EProjectLaunchAction PromptForLaunchAction() = 0;

    /**
     * @brief Ask the user to select a project file.
     * @param outProjectFilePath Absolute path to chosen .jproject file.
     * @return True if the user selected one.
     */
    virtual bool PromptForProjectFile(std::string& outProjectFilePath) = 0;

    /**
     * @brief Ask the user to select an engine root directory or executable.
     * @param projectFilePath Project that needs an engine association fixed.
     * @param outEnginePath Engine root or executable chosen by user.
     * @return True if the user selected one.
     */
    virtual bool PromptForEnginePath(const std::string& projectFilePath, std::string& outEnginePath) = 0;

    virtual bool PromptForNewProject(FProjectCreateRequest& outRequest) = 0;

    /**
     * @brief Show an error message.
     */
    virtual void ShowError(const std::string& title, const std::string& message) = 0;

    virtual void Shutdown() {}
};

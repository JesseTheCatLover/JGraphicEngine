// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "ProjectLaunch/ConsoleProjectLaunchUI.h"

#include <iostream>

EProjectLaunchAction ConsoleProjectLaunchUI::PromptForLaunchAction()
{
    std::cout << "[Launch]: Choose action:\n";
    std::cout << "  1) Open existing project\n";
    std::cout << "  2) Create new project\n";
    std::cout << "  0) Cancel\n";
    std::cout << "> ";

    std::string input;
    std::getline(std::cin, input);

    if (input == "1") return EProjectLaunchAction::OpenExisting;
    if (input == "2") return EProjectLaunchAction::CreateNew;
    return EProjectLaunchAction::Cancel;
}

bool ConsoleProjectLaunchUI::PromptForProjectFile(std::string& outProjectFilePath)
{
    outProjectFilePath.clear();

    std::cout << "[Launch]: Enter .jproject path: ";
    std::getline(std::cin, outProjectFilePath);

    return !outProjectFilePath.empty();
}

bool ConsoleProjectLaunchUI::PromptForEnginePath(const std::string& projectFilePath, std::string& outEnginePath)
{
    outEnginePath.clear();

    std::cout << "[Launch]: Could not resolve engine for project: " << projectFilePath << "\n";
    std::cout << "[Launch]: Enter engine root directory or executable path: ";
    std::getline(std::cin, outEnginePath);

    return !outEnginePath.empty();
}

bool ConsoleProjectLaunchUI::PromptForNewProject(FProjectCreateRequest& outRequest)
{
    outRequest = {};

    std::cout << "[Launch]: Enter new project name: ";
    std::getline(std::cin, outRequest.projectName);

    std::cout << "[Launch]: Enter parent directory for the project: ";
    std::getline(std::cin, outRequest.parentDirectory);

    return !outRequest.projectName.empty() && !outRequest.parentDirectory.empty();
}

void ConsoleProjectLaunchUI::ShowError(const std::string& title, const std::string& message)
{
    std::cerr << "[" << title << "]: " << message << "\n";
}

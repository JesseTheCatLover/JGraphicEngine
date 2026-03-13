// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "ProjectLaunch/ConsoleProjectLaunchUI.h"

#include <iostream>

bool ConsoleProjectLaunchUI::PromptForProjectFile(std::string& outProjectFilePath)
{
    outProjectFilePath.clear();

    std::cout << "[Launch] Enter .jproject path: ";
    std::getline(std::cin, outProjectFilePath);

    return !outProjectFilePath.empty();
}

bool ConsoleProjectLaunchUI::PromptForEnginePath(const std::string& projectFilePath, std::string& outEnginePath)
{
    outEnginePath.clear();

    std::cout << "[Launch] Could not resolve engine for project: " << projectFilePath << "\n";
    std::cout << "[Launch] Enter engine root directory or executable path: ";
    std::getline(std::cin, outEnginePath);

    return !outEnginePath.empty();
}

void ConsoleProjectLaunchUI::ShowError(const std::string& title, const std::string& message)
{
    std::cerr << "[" << title << "]: " << message << "\n";
}
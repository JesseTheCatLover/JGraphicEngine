// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <string>

enum class EProjectLaunchSource : uint8_t
{
    DirectEngineExecutable,
    ProjectFileAssociation,
    Launcher
};

struct FProjectOpenRequest
{
    EProjectLaunchSource launchSource = EProjectLaunchSource::DirectEngineExecutable;

    std::string projectFilePath; // path to .jproject
    std::string engineRootPath;  // resolved engine root to use
};
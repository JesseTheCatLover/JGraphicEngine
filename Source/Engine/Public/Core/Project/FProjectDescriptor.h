// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

struct FEngineAssociation
{
    std::string identifier;          // e.g. "Stable-1.2"
    std::string lastKnownEnginePath; // optional, machine-local convenience path
};

struct FProjectDescriptor
{
    static constexpr int32_t CurrentVersion = 1;

    int32_t projectVersion = CurrentVersion;

    std::string projectName;
    std::string projectID;

    std::string description;

    std::string thumbnailRelativePath;

    FEngineAssociation engineAssociation;

    struct FFolders
    {
        std::string assets = "Assets";
        std::string saved = "Saved";
        std::string intermediate = "Intermediate";
        std::string config = "Configs";
    } folders;

    std::string startupScene;
};
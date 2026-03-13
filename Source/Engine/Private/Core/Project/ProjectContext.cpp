// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Core/Project/ProjectContext.h"

#include <iostream>

#include "Core/Serialization/JsonReader.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

bool ProjectContext::OpenProject(const FProjectOpenRequest& request)
{
    Reset();

    const std::string normalizedProjectFile = UPath::Normalize(request.projectFilePath);
    const std::string normalizedEngineRoot  = UPath::Normalize(request.engineRootPath);

    if (!UFileSystem::FileExists(normalizedProjectFile))
    {
        std::cerr << "[ProjectContext]: .jproject file does not exist: " << normalizedProjectFile << "\n";
        return false;
    }

    if (!UFileSystem::DirectoryExists(normalizedEngineRoot))
    {
        std::cerr << "[ProjectContext]: Engine root does not exist: " << normalizedEngineRoot << "\n";
        return false;
    }

    FProjectDescriptor loadedDescriptor;
    if (!LoadDescriptorFile(normalizedProjectFile, loadedDescriptor))
        return false;

    if (!ValidateDescriptor(loadedDescriptor))
        return false;

    m_OpenRequest     = request;
    m_ProjectFilePath = normalizedProjectFile;
    m_ProjectRoot     = UPath::GetParent(normalizedProjectFile);
    m_EngineRoot      = normalizedEngineRoot;
    m_Descriptor      = std::move(loadedDescriptor);

    BuildResolvedPaths();

    m_bIsOpen = true;
    return true;
}

void ProjectContext::Reset()
{
    m_bIsOpen = false;

    m_Descriptor = FProjectDescriptor{};
    m_OpenRequest = FProjectOpenRequest{};

    m_ProjectFilePath.clear();
    m_ProjectRoot.clear();

    m_ProjectAssetsRoot.clear();
    m_ProjectSavedRoot.clear();
    m_ProjectIntermediateRoot.clear();
    m_ProjectConfigRoot.clear();

    m_EngineRoot.clear();
    m_EngineAssetsRoot.clear();
}

bool ProjectContext::LoadDescriptorFile(const std::string& projectFilePath, FProjectDescriptor& outDescriptor) const
{
    JsonReader reader;
    if (!reader.LoadFromFile(projectFilePath) || !reader.IsValid())
    {
        std::cerr << "[ProjectContext]: Failed to load .jproject file: " << projectFilePath << "\n";
        return false;
    }

    outDescriptor.projectVersion = reader.Read<int32_t>("projectVersion", FProjectDescriptor::CurrentVersion);
    outDescriptor.projectName    = reader.Read<std::string>("projectName", "");
    outDescriptor.projectID      = reader.Read<std::string>("projectID", "");
    outDescriptor.startupScene   = reader.Read<std::string>("startupScene", "");

    if (reader.IsObject("engineAssociation"))
    {
        JsonReader assoc = reader.GetObject("engineAssociation");
        outDescriptor.engineAssociation.identifier =
            assoc.Read<std::string>("identifier", "");
        outDescriptor.engineAssociation.lastKnownEnginePath =
            assoc.Read<std::string>("lastKnownEnginePath", "");
    }

    if (reader.IsObject("folders"))
    {
        JsonReader folders = reader.GetObject("folders");
        outDescriptor.folders.assets =
            folders.Read<std::string>("assets", "Assets");
        outDescriptor.folders.saved =
            folders.Read<std::string>("saved", "Saved");
        outDescriptor.folders.intermediate =
            folders.Read<std::string>("intermediate", "Intermediate");
        outDescriptor.folders.config =
            folders.Read<std::string>("config", "Config");
    }

    return true;
}

bool ProjectContext::ValidateDescriptor(const FProjectDescriptor& descriptor) const
{
    if (descriptor.projectVersion <= 0)
    {
        std::cerr << "[ProjectContext]: Invalid projectVersion in .jproject\n";
        return false;
    }

    if (descriptor.projectVersion > FProjectDescriptor::CurrentVersion)
    {
        std::cerr << "[ProjectContext]: Unsupported future .jproject version: "
                  << descriptor.projectVersion << "\n";
        return false;
    }

    if (descriptor.projectName.empty())
    {
        std::cerr << "[ProjectContext]: projectName is empty in .jproject\n";
        return false;
    }

    if (descriptor.projectID.empty())
    {
        std::cerr << "[ProjectContext]: projectID is empty in .jproject\n";
        return false;
    }

    if (descriptor.folders.assets.empty())
    {
        std::cerr << "[ProjectContext]: folders.assets is empty in .jproject\n";
        return false;
    }

    return true;
}

void ProjectContext::BuildResolvedPaths()
{
    m_ProjectAssetsRoot       = UPath::Normalize(UPath::Join(m_ProjectRoot, m_Descriptor.folders.assets));
    m_ProjectSavedRoot        = UPath::Normalize(UPath::Join(m_ProjectRoot, m_Descriptor.folders.saved));
    m_ProjectIntermediateRoot = UPath::Normalize(UPath::Join(m_ProjectRoot, m_Descriptor.folders.intermediate));
    m_ProjectConfigRoot       = UPath::Normalize(UPath::Join(m_ProjectRoot, m_Descriptor.folders.config));

    m_EngineAssetsRoot        = UPath::Normalize(UPath::Join(m_EngineRoot, "Assets"));
}
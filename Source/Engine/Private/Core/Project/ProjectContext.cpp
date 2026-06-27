// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Core/Project/ProjectContext.h"

#include <iostream>

#include "Core/Serialization/JsonReader.h"
#include "Core/Serialization/JsonWriter.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"
#include "Utilities/UUUID.h"

bool ProjectContext::OpenProject(const FProjectOpenRequest& request)
{
    Reset();

    const std::string normalizedProjectFile = UPath::NormalizePhysical(request.projectFilePath);
    const std::string normalizedEngineRoot  = UPath::NormalizePhysical(request.engineRootPath);

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
    if (!LoadProjectDescriptor(normalizedProjectFile, loadedDescriptor))
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

bool ProjectContext::CreateProject(const FProjectCreateRequest &request, FProjectCreateResult &outResult)
{
    outResult = {};

    if (!IsValidProjectName(request.projectName))
    {
        outResult.errors.emplace_back("Project name is invalid. Use letters, digits, '_' or '-'.");
        return false;
    }

    if (request.parentDirectory.empty())
    {
        outResult.errors.emplace_back("Parent directory is empty.");
        return false;
    }

    if (request.engineRootPath.empty())
    {
        outResult.errors.emplace_back("Engine root path is empty.");
        return false;
    }

    const std::string normalizedParentDir  = UPath::NormalizePhysical(request.parentDirectory);
    const std::string normalizedEngineRoot = UPath::NormalizePhysical(request.engineRootPath);

    if (!UFileSystem::DirectoryExists(normalizedParentDir))
    {
        if (!UFileSystem::CreateDirectory(normalizedParentDir))
        {
            outResult.errors.emplace_back("Failed to create parent directory.");
            return false;
        }
    }

    if (!UFileSystem::DirectoryExists(normalizedEngineRoot))
    {
        outResult.errors.emplace_back("Engine root directory does not exist.");
        return false;
    }

    const std::string projectRootPath   = UPath::NormalizePhysical(UPath::Join(normalizedParentDir, request.projectName));
    const std::string assetsDir         = UPath::NormalizePhysical(UPath::Join(projectRootPath, "Assets"));
    const std::string savedDir          = UPath::NormalizePhysical(UPath::Join(projectRootPath, "Saved"));
    const std::string intermediateDir   = UPath::NormalizePhysical(UPath::Join(projectRootPath, "Intermediate"));
    const std::string configDir         = UPath::NormalizePhysical(UPath::Join(projectRootPath, "Configs"));
    const std::string projectFilePath   = UPath::NormalizePhysical(UPath::Join(projectRootPath, request.projectName + ".jproject"));

    if (UFileSystem::DirectoryExists(projectRootPath) || UFileSystem::FileExists(projectFilePath))
    {
        outResult.errors.emplace_back("Project directory already exists.");
        return false;
    }

    if (!UFileSystem::CreateDirectory(projectRootPath))
    {
        outResult.errors.emplace_back("Failed to create project root directory.");
        return false;
    }

    if (!UFileSystem::CreateDirectory(assetsDir) ||
        !UFileSystem::CreateDirectory(savedDir) ||
        !UFileSystem::CreateDirectory(intermediateDir) ||
        !UFileSystem::CreateDirectory(configDir))
    {
        outResult.errors.emplace_back("Failed to create project directory structure.");
        return false;
    }

    JsonWriter writer;
    writer.Write("projectVersion", FProjectDescriptor::CurrentVersion);
    writer.Write("projectName", request.projectName);
    writer.Write("projectID", UUUID::GenerateUUID());

    writer.Write("description", "");
    writer.Write("thumbnailRelativePath", "");

    writer.BeginObject("engineAssociation");
    writer.Write("lastKnownEnginePath", normalizedEngineRoot);
    writer.EndObject();

    writer.BeginObject("folders");
    writer.Write("assets", "Assets");
    writer.Write("saved", "Saved");
    writer.Write("intermediate", "Intermediate");
    writer.Write("config", "Config");
    writer.EndObject();

    if (!writer.SaveToFile(projectFilePath))
    {
        outResult.errors.emplace_back("Failed to write .jproject file.");
        return false;
    }

    outResult.bSuccess = true;
    outResult.projectRootPath = projectRootPath;
    outResult.projectFilePath = projectFilePath;
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

bool ProjectContext::IsValidProjectName(const std::string& name)
{
    if (name.empty())
        return false;

    for (char c : name)
    {
        const bool bValid =
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '_' || c == '-';

        if (!bValid)
            return false;
    }

    return true;
}

bool ProjectContext::LoadProjectDescriptor(const std::string& projectFilePath, FProjectDescriptor& outDescriptor)
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
    outDescriptor.description    = reader.Read<std::string>("description", "");
    outDescriptor.thumbnailRelativePath = reader.Read<std::string>("thumbnailRelativePath", "");

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
    m_ProjectAssetsRoot       = UPath::NormalizePhysical(UPath::Join(m_ProjectRoot, m_Descriptor.folders.assets));
    m_ProjectSavedRoot        = UPath::NormalizePhysical(UPath::Join(m_ProjectRoot, m_Descriptor.folders.saved));
    m_ProjectIntermediateRoot = UPath::NormalizePhysical(UPath::Join(m_ProjectRoot, m_Descriptor.folders.intermediate));
    m_ProjectConfigRoot       = UPath::NormalizePhysical(UPath::Join(m_ProjectRoot, m_Descriptor.folders.config));

    m_EngineAssetsRoot        = UPath::NormalizePhysical(UPath::Join(m_EngineRoot, "Assets"));
}
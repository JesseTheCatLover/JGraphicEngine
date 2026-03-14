// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "ProjectLaunch/ProjectLaunchResolver.h"

#include "Core/Serialization/JsonReader.h"
#include "Core/Serialization/JsonWriter.h"
#include "ProjectLaunch/EngineInstallResolver.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

ProjectLaunchResolver::ProjectLaunchResolver(IProjectLaunchUI& ui)
    : m_UI(ui)
{
    m_EngineResolver = &m_OwnedEngineResolver;
}

bool ProjectLaunchResolver::ResolveDirectLaunch(const std::string& preferredProjectFilePath,
                                                const std::string& currentEngineRoot,
                                                FProjectOpenRequest& outRequest)
{
    outRequest = {};

    std::string projectFilePath = preferredProjectFilePath;
    if (projectFilePath.empty())
    {
        if (!m_UI.PromptForProjectFile(projectFilePath))
            return false;
    }

    if (!UFileSystem::FileExists(projectFilePath))
    {
        m_UI.ShowError("Launch", "Selected .jproject file does not exist.");
        return false;
    }

    outRequest.launchSource   = EProjectLaunchSource::DirectEngineExecutable;
    outRequest.projectFilePath= UPath::Normalize(projectFilePath);
    outRequest.engineRootPath = UPath::Normalize(currentEngineRoot);
    return true;
}

bool ProjectLaunchResolver::ResolveProjectFileLaunch(const std::string& projectFilePath,
                                                     FProjectOpenRequest& outRequest)
{
    outRequest = {};

    const std::string normalizedProjectFile = UPath::Normalize(projectFilePath);
    if (!UFileSystem::FileExists(normalizedProjectFile))
    {
        m_UI.ShowError("Launch", ".jproject file does not exist.");
        return false;
    }

    FProjectDescriptor descriptor;
    if (!LoadProjectDescriptor(normalizedProjectFile, descriptor))
    {
        m_UI.ShowError("Launch", "Failed to read .jproject.");
        return false;
    }

    if (!TryResolveEngineForProject(normalizedProjectFile, descriptor, outRequest))
        return false;

    outRequest.launchSource    = EProjectLaunchSource::ProjectFileAssociation;
    outRequest.projectFilePath = normalizedProjectFile;
    return true;
}

bool ProjectLaunchResolver::ResolveLauncherLaunch(const std::string& projectFilePath,
                                                  const std::string& engineRootPath,
                                                  FProjectOpenRequest& outRequest)
{
    outRequest = {};

    FResolvedEngineInstall resolved;
    if (!m_EngineResolver->Resolve(engineRootPath, resolved))
    {
        m_UI.ShowError("Launch", "Launcher provided an invalid engine path.");
        return false;
    }

    if (!UFileSystem::FileExists(projectFilePath))
    {
        m_UI.ShowError("Launch", "Launcher provided an invalid .jproject path.");
        return false;
    }

    outRequest.launchSource    = EProjectLaunchSource::Launcher;
    outRequest.projectFilePath = UPath::Normalize(projectFilePath);
    outRequest.engineRootPath  = resolved.engineRootPath;
    return true;
}

bool ProjectLaunchResolver::LoadProjectDescriptor(const std::string& projectFilePath,
                                                  FProjectDescriptor& outDescriptor) const
{
    JsonReader reader;
    if (!reader.LoadFromFile(projectFilePath) || !reader.IsValid())
        return false;

    outDescriptor = {};

    outDescriptor.projectVersion = reader.Read<int32_t>("projectVersion", FProjectDescriptor::CurrentVersion);
    outDescriptor.projectName    = reader.Read<std::string>("projectName", "");
    outDescriptor.projectID      = reader.Read<std::string>("projectID", "");
    outDescriptor.startupScene   = reader.Read<std::string>("startupScene", "");

    if (reader.IsObject("engineAssociation"))
    {
        JsonReader assoc = reader.GetObject("engineAssociation");
        outDescriptor.engineAssociation.lastKnownEnginePath =
            assoc.Read<std::string>("lastKnownEnginePath", "");
    }

    if (reader.IsObject("folders"))
    {
        JsonReader folders = reader.GetObject("folders");
        outDescriptor.folders.assets       = folders.Read<std::string>("assets", "Assets");
        outDescriptor.folders.saved        = folders.Read<std::string>("saved", "Saved");
        outDescriptor.folders.intermediate = folders.Read<std::string>("intermediate", "Intermediate");
        outDescriptor.folders.config       = folders.Read<std::string>("config", "Config");
    }

    return true;
}

bool ProjectLaunchResolver::TryResolveEngineForProject(const std::string& projectFilePath,
                                                       const FProjectDescriptor& descriptor,
                                                       FProjectOpenRequest& outRequest)
{
    FResolvedEngineInstall resolved;

    const std::string& lastKnown = descriptor.engineAssociation.lastKnownEnginePath;
    if (!lastKnown.empty() && m_EngineResolver->Resolve(lastKnown, resolved))
    {
        outRequest.engineRootPath = resolved.engineRootPath;
        return true;
    }

    std::string userChosenPath;
    if (!m_UI.PromptForEnginePath(projectFilePath, userChosenPath))
        return false;

    if (!m_EngineResolver->Resolve(userChosenPath, resolved))
    {
        m_UI.ShowError("Launch", "Selected engine path is not valid.");
        return false;
    }

    if (!UpdateProjectLastKnownEnginePath(projectFilePath, resolved.engineRootPath))
    {
        m_UI.ShowError("Launch", "Failed to update project's lastKnownEnginePath.");
        return false;
    }

    outRequest.engineRootPath = resolved.engineRootPath;
    return true;
}

bool ProjectLaunchResolver::UpdateProjectLastKnownEnginePath(const std::string& projectFilePath,
                                                             const std::string& engineRootPath) const
{
    JsonReader reader;
    if (!reader.LoadFromFile(projectFilePath) || !reader.IsValid())
        return false;

    JJson data = reader.GetData();

    if (!data.contains("engineAssociation") || !data["engineAssociation"].is_object())
        data["engineAssociation"] = JJson::object();

    data["engineAssociation"]["lastKnownEnginePath"] = engineRootPath;

    JsonWriter writer;
    writer.GetData() = data;
    return writer.SaveToFile(projectFilePath);
}
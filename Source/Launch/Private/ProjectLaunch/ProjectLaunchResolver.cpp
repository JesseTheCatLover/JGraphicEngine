// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "ProjectLaunch/ProjectLaunchResolver.h"

#include "Core/Project/ProjectContext.h"
#include "Core/Serialization/JsonReader.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Project/LaunchSettings.h"
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

    // 1) Explicit --project path always wins
    if (!preferredProjectFilePath.empty())
    {
        if (!UFileSystem::FileExists(preferredProjectFilePath))
        {
            m_UI.ShowError("Launch", "Provided .jproject file does not exist.");
            return false;
        }

        outRequest.launchSource    = EProjectLaunchSource::DirectEngineExecutable;
        outRequest.projectFilePath = UPath::NormalizePhysical(preferredProjectFilePath);
        outRequest.engineRootPath  = UPath::NormalizePhysical(currentEngineRoot);
        return true;
    }

    // 2) Try last opened project from launch settings
    {
        LaunchSettings settings;
        if (settings.Load(currentEngineRoot))
        {
            if (settings.GetShouldOpenLastProjectOnStartup()) // Only open recent project if settings let us
            {
                const auto& recentList = settings.GetRecentProjectPaths();
                if (!recentList.empty())
                {
                    // Index 0 is always the most recent
                    const std::string& lastProject = recentList[0];
                    if (UFileSystem::FileExists(lastProject))
                    {
                        outRequest.launchSource    = EProjectLaunchSource::DirectEngineExecutable;
                        outRequest.projectFilePath = UPath::NormalizePhysical(lastProject);
                        outRequest.engineRootPath  = UPath::NormalizePhysical(currentEngineRoot);
                        return true;
                    }
                }
            }
        }
    }

    // 3) Fall back to prompt
    const EProjectLaunchAction action = m_UI.PromptForLaunchAction();
    if (action == EProjectLaunchAction::Cancel)
        return false;

    if (action == EProjectLaunchAction::OpenExisting)
    {
        std::string projectFilePath;
        if (!m_UI.PromptForProjectFile(projectFilePath))
            return false;

        if (!UFileSystem::FileExists(projectFilePath))
        {
            m_UI.ShowError("Launch", "Selected .jproject file does not exist.");
            return false;
        }

        outRequest.launchSource    = EProjectLaunchSource::DirectEngineExecutable;
        outRequest.projectFilePath = UPath::NormalizePhysical(projectFilePath);
        outRequest.engineRootPath  = UPath::NormalizePhysical(currentEngineRoot);
        return true;
    }

    if (action == EProjectLaunchAction::CreateNew)
    {
        FProjectCreateRequest createRequest;
        if (!m_UI.PromptForNewProject(createRequest))
            return false;

        createRequest.engineRootPath = currentEngineRoot;

        FProjectCreateResult createResult;
        if (!ProjectContext::CreateProject(createRequest, createResult))
        {
            std::string error = "Failed to create project.";
            if (!createResult.errors.empty())
                error += " " + createResult.errors.front();

            m_UI.ShowError("Launch", error);
            return false;
        }

        outRequest.launchSource    = EProjectLaunchSource::DirectEngineExecutable;
        outRequest.projectFilePath = UPath::NormalizePhysical(createResult.projectFilePath);
        outRequest.engineRootPath  = UPath::NormalizePhysical(currentEngineRoot);
        return true;
    }

    return false;
}

bool ProjectLaunchResolver::ResolveProjectFileLaunch(const std::string& projectFilePath,
                                                     FProjectOpenRequest& outRequest)
{
    outRequest = {};

    const std::string normalizedProjectFile = UPath::NormalizePhysical(projectFilePath);
    if (!UFileSystem::FileExists(normalizedProjectFile))
    {
        m_UI.ShowError("Launch", ".jproject file does not exist.");
        return false;
    }

    FProjectDescriptor descriptor;
    if (!FProjectDescriptor::LoadFromFile(normalizedProjectFile, descriptor))
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
    outRequest.projectFilePath = UPath::NormalizePhysical(projectFilePath);
    outRequest.engineRootPath  = resolved.engineRootPath;
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
                                                             const std::string& engineRootPath) const // TODO: Later add engine version update too
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
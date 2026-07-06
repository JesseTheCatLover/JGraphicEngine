// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

#include "EngineInstallResolver.h"
#include "IProjectLaunchUI.h"
#include "Core/Project/FProjectDescriptor.h"
#include "Core/Project/FProjectOpenRequest.h"

class ProjectLaunchUI;
class EngineInstallResolver;

class ProjectLaunchResolver
{
public:
    explicit ProjectLaunchResolver(IProjectLaunchUI& ui);

    /**
     * @brief Resolve startup when engine was launched directly.
     *
     * Current v1 behavior:
     * - if preferredProjectFilePath is provided, use it
     * - otherwise prompt the user
     */
    bool ResolveDirectLaunch(const std::string& preferredProjectFilePath,
                             const std::string& currentEngineRoot,
                             bool bForceLauncher,
                             FProjectOpenRequest& outRequest);

    /**
     * @brief Resolve startup when launched from a .jproject file association.
     */
    bool ResolveProjectFileLaunch(const std::string& projectFilePath,
                                  FProjectOpenRequest& outRequest);

    /**
     * @brief Resolve startup when launched from a future launcher.
     */
    bool ResolveLauncherLaunch(const std::string& projectFilePath,
                               const std::string& engineRootPath,
                               FProjectOpenRequest& outRequest);

private:
    bool TryResolveEngineForProject(const std::string& projectFilePath,
                                    const FProjectDescriptor& descriptor,
                                    FProjectOpenRequest& outRequest);

    bool UpdateProjectLastKnownEnginePath(const std::string& projectFilePath,
                                          const std::string& engineRootPath) const;

private:
    IProjectLaunchUI& m_UI;
    EngineInstallResolver* m_EngineResolver = nullptr;
    EngineInstallResolver  m_OwnedEngineResolver;
};
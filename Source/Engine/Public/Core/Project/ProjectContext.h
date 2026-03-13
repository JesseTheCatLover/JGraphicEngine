// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

#include "FProjectDescriptor.h"
#include "FProjectOpenRequest.h"

class ProjectContext
{
public:
    ProjectContext() = default;
    ~ProjectContext() = default;

    ProjectContext(const ProjectContext&) = delete;
    ProjectContext& operator=(const ProjectContext&) = delete;
    ProjectContext(ProjectContext&&) = delete;
    ProjectContext& operator=(ProjectContext&&) = delete;

public:
    bool OpenProject(const FProjectOpenRequest& request);
    void Reset();

    [[nodiscard]] bool IsOpen() const { return m_bIsOpen; }

    [[nodiscard]] const FProjectDescriptor& GetDescriptor() const { return m_Descriptor; }

    [[nodiscard]] const std::string& GetProjectName() const { return m_Descriptor.projectName; }
    [[nodiscard]] const std::string& GetProjectID() const { return m_Descriptor.projectID; }
    [[nodiscard]] const std::string& GetStartupScene() const { return m_Descriptor.startupScene; }

    [[nodiscard]] const std::string& GetProjectFilePath() const { return m_ProjectFilePath; }
    [[nodiscard]] const std::string& GetProjectRoot() const { return m_ProjectRoot; }
    [[nodiscard]] const std::string& GetProjectAssetsRoot() const { return m_ProjectAssetsRoot; }
    [[nodiscard]] const std::string& GetProjectSavedRoot() const { return m_ProjectSavedRoot; }
    [[nodiscard]] const std::string& GetProjectIntermediateRoot() const { return m_ProjectIntermediateRoot; }
    [[nodiscard]] const std::string& GetProjectConfigRoot() const { return m_ProjectConfigRoot; }

    [[nodiscard]] const std::string& GetEngineRoot() const { return m_EngineRoot; }
    [[nodiscard]] const std::string& GetEngineAssetsRoot() const { return m_EngineAssetsRoot; }

private:
    bool LoadDescriptorFile(const std::string& projectFilePath, FProjectDescriptor& outDescriptor) const;
    bool ValidateDescriptor(const FProjectDescriptor& descriptor) const;
    void BuildResolvedPaths();

private:
    bool m_bIsOpen = false;

    FProjectDescriptor m_Descriptor;
    FProjectOpenRequest m_OpenRequest;

    std::string m_ProjectFilePath;
    std::string m_ProjectRoot;

    std::string m_ProjectAssetsRoot;
    std::string m_ProjectSavedRoot;
    std::string m_ProjectIntermediateRoot;
    std::string m_ProjectConfigRoot;

    std::string m_EngineRoot;
    std::string m_EngineAssetsRoot;
};
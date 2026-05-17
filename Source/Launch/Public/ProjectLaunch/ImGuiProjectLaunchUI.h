//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "ProjectLaunch/IProjectLaunchUI.h"
#include <string>
#include <optional>
#include <filesystem>

#include "Core/Memory/SmartPointers.h"

enum class EBrowserCategory
{
    RecentProjects,
    Games,
    Animation
};

// Represents a project or template card in the UI
struct FBrowserItem
{
    std::string Name;
    std::string Path;
    std::string Description;
    bool bIsTemplate = false;
    // void* PreviewTextureID = nullptr; // Uncomment later when implementing image thumbnails
};

class IRenderBackend;
class IPlatformWindow;
class IPlatformSurface;

class ImGuiProjectLaunchUI : public IProjectLaunchUI
{
private:
    // --- Window & Rendering ---
    bool m_windowInitialized = false;
    TSharedPtr<IPlatformWindow> m_Window = nullptr;
    IPlatformSurface* m_Surface = nullptr;
    IRenderBackend* m_RenderBackend = nullptr;
    std::string m_EngineRootPath;

    // --- Error popup panel ---
    bool m_ShowingError = false;
    std::string m_ErrorTitle, m_ErrorMessage;

    // --- Unified UI State ---
    EBrowserCategory m_SelectedCategory = EBrowserCategory::RecentProjects;
    int m_SelectedItemIndex = -1;

    // Creation Buffers
    char m_NewProjectName[256] = "MyProject";
    char m_NewProjectPath[1024] = "";

    // Data Models
    std::vector<FBrowserItem> m_RecentProjects;
    std::vector<FBrowserItem> m_Templates;

    // --- Cached Results ---
    // We store the results here when the unified browser closes,
    // so we can hand them to the Engine when it asks.
    std::string m_CachedProjectToOpen;
    FProjectCreateRequest m_CachedCreateRequest;

public:
    explicit ImGuiProjectLaunchUI(IPlatformSurface* surface, IRenderBackend* renderBackend, std::string engineRootPath);
    ~ImGuiProjectLaunchUI() override;

    EProjectLaunchAction PromptForLaunchAction() override;
    bool PromptForProjectFile(std::string& outProjectFilePath) override;
    bool PromptForEnginePath(const std::string& projectFilePath, std::string& outEnginePath) override;
    bool PromptForNewProject(FProjectCreateRequest& outRequest) override;
    void ShowError(const std::string& title, const std::string& message) override;
    void Shutdown() override;

private:
    bool StartWindow();
    void ShutdownWindow();
    void SetupLauncherStyle();
    void SetupFonts();

    // The single unified loop that replaces all the old separate screens
    EProjectLaunchAction RunUnifiedBrowserLoop();

    // --- Unified Layout Sections ---
    void DrawSidebar();
    void DrawContentGrid();
    void DrawDetailsPane();
    void DrawBottomBar(EProjectLaunchAction& outAction, bool& outFinished);

    // --- Popups ---
    bool DrawEnginePathPicker(std::string& outEnginePath);
    void DrawErrorPopup(const std::string& title, const std::string& message);

    // --- Data Population & Utilities ---
    void RefreshRecentProjects();
    void LoadTemplates();

    // Helpers:
    // Recursively finds a .jproject file inside a selected directory
    bool SearchForProjectFile(const std::filesystem::path& folderPath, std::string& outProjectPath);
};

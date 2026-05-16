//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "ProjectLaunch/IProjectLaunchUI.h"
#include <string>
#include <optional>
#include <filesystem>

#include "Core/Memory/SmartPointers.h"

class IRenderBackend;
class IPlatformWindow;
class IPlatformSurface;

class ImGuiProjectLaunchUI : public IProjectLaunchUI
{
private:
    bool m_windowInitialized = false;
    TSharedPtr<IPlatformWindow> m_Window = nullptr;
    IPlatformSurface* m_Surface = nullptr;
    IRenderBackend* m_RenderBackend = nullptr;

public:
    explicit ImGuiProjectLaunchUI(IPlatformSurface* surface, IRenderBackend* renderBackend);
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

    // One generic modal loop that draws a specific UI "mode"
    template<typename DrawFunc>
    bool RunModalLoop(DrawFunc drawFunc);

    // UI Draw mode Functions
    bool DrawLaunchActionScreen(EProjectLaunchAction& outAction);
    bool DrawProjectFilePicker(std::string& outProjectFilePath);
    bool DrawEnginePathPicker(const std::string& projectFilePath, std::string& outEnginePath);
    bool DrawNewProjectScreen(FProjectCreateRequest& outRequest);
    void DrawErrorPopup(const std::string& title, const std::string& message);
};

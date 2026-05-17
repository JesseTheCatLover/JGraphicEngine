//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "ImGuiBackend.h"

#include <fstream>
#include <sstream>
#include <string>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Core/EngineGlobals.h"
#include "Core/Project/ProjectContext.h"
#include "Core/Project/VirtualPathMounter.h"
#include "UI/Themes/ImGuiTheme.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

namespace
{
    std::filesystem::path GetCurrentLayoutPath()
    {
        if (!GEngine)
            return {};

        const std::string savedRoot = GEngine->GetProjectContext()->GetProjectSavedRoot();
        return UPath::Join(savedRoot, "Editor", "Layout", "ImGui", "CurrentLayout.ini");
    }
    bool ReadTextFile(const std::filesystem::path& path, std::string& outText)
    {
        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file.is_open())
            return false;

        std::ostringstream ss;
        ss << file.rdbuf();
        outText = ss.str();
        return true;
    }

    bool WriteTextFile(const std::filesystem::path& path, const char* data, size_t size)
    {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);

        std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file.is_open())
            return false;

        file.write(data, static_cast<std::streamsize>(size));
        return file.good();
    }
}

ImGuiBackend::ImGuiBackend(GLFWwindow* window)
    : m_Window(window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // manual save/load

    SetupFonts();
    SetupStyle();

    m_HasLoadedSettings = LoadSettings();

    ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

ImGuiBackend::~ImGuiBackend()
{
}

void ImGuiBackend::BeginFrame()
{
    // Optional: auto-save only when ImGui says settings changed
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantSaveIniSettings)
    {
        SaveSettings();
        io.WantSaveIniSettings = false;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiBackend::EndFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiBackend::Shutdown()
{
    SaveSettings();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiBackend::SetupFonts()
{
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    std::string defaultFontName = "FunnelSans";
    std::string fontPath;
    if (GEngine && GEngine->GetVirtualPathMounter().ResolveVirtualToPhysical(
            UPath::Join("/Engine/Editor/Fonts", defaultFontName + ".ttf"), fontPath))
    {
        io.FontDefault = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f);
    }
}

void ImGuiBackend::SetupStyle()
{
    ImGuiTheme::FThemeOptions themeOptions;
    themeOptions.enableDocking = true;
    ImGuiTheme::ApplyEditorTheme(themeOptions);
}

bool ImGuiBackend::LoadSettings()
{
    const std::filesystem::path layoutPath = GetCurrentLayoutPath();
    if (!std::filesystem::exists(layoutPath))
        return false;

    ImGui::LoadIniSettingsFromDisk(layoutPath.string().c_str());
    return true;
}

void ImGuiBackend::SaveSettings()
{
    const std::filesystem::path layoutPath = GetCurrentLayoutPath();

    std::error_code ec;
    std::filesystem::create_directories(layoutPath.parent_path(), ec);

    ImGui::SaveIniSettingsToDisk(layoutPath.string().c_str());
}
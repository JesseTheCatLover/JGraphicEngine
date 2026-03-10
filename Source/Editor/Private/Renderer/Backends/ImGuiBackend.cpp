//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "ImGuiBackend.h"

#include <fstream>
#include <sstream>
#include <string>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

namespace
{
    std::filesystem::path GetCurrentLayoutPath()
    {
        return UPath::ResolvePath(UPath::Join("Config", "Layout", "current_layout.ini"));
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
    io.FontDefault = io.Fonts->AddFontFromFileTTF(
        UPath::ResolvePath(UPath::Join("Assets/Editor", "Fonts", defaultFontName + ".ttf")).string().c_str(),
        16.0f
    );
}

void ImGuiBackend::SetupStyle()
{
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGuiStyle& style = ImGui::GetStyle();

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    style.WindowMenuButtonPosition = ImGuiDir_None;

    style.WindowRounding = 3.0f;
    style.FrameRounding  = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing  = ImVec2(10, 4);

    ImVec4* colors = style.Colors;

    ImVec4 bg_dark   = ImVec4(0.17f, 0.17f, 0.17f, 1.0f);
    ImVec4 bg_mid    = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    ImVec4 bg_light  = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
    ImVec4 text_col  = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);

    ImVec4 accent        = ImVec4(0.43f, 0.51f, 0.59f, 1.0f);
    ImVec4 accent_active = ImVec4(0.34f, 0.42f, 0.50f, 1.0f);

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.45f, 0.45f, 0.5f);
    style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.2f);
    style.Colors[ImGuiCol_Button]   = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive]  = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);

    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.37f, 0.48f, 0.58f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = accent_active;

    style.Colors[ImGuiCol_Text]         = text_col;
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
    colors[ImGuiCol_WindowBg]         = bg_dark;
    colors[ImGuiCol_ChildBg]          = bg_dark;
    colors[ImGuiCol_PopupBg]          = bg_mid;
    colors[ImGuiCol_Border]           = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);
    colors[ImGuiCol_BorderShadow]     = ImVec4(0, 0, 0, 0);

    colors[ImGuiCol_TitleBg]          = bg_dark;
    colors[ImGuiCol_TitleBgActive]    = bg_mid;
    colors[ImGuiCol_TitleBgCollapsed] = bg_mid;

    colors[ImGuiCol_Tab]                  = bg_mid;
    colors[ImGuiCol_TabHovered]           = bg_light;
    colors[ImGuiCol_TabActive]            = bg_light;
    colors[ImGuiCol_TabUnfocused]         = bg_mid;
    colors[ImGuiCol_TabUnfocusedActive]   = bg_light;

    style.TabRounding = 3.0f;
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
//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "ProjectLaunch/ImGuiProjectLaunchUI.h"

#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "UI/Themes/ImGuiTheme.h"
#include "Rendering/IRenderBackend.h"
#include "Rendering/IPlatformSurface.h"
#include "Rendering/IPlatformWindow.h"
#include "Utilities/UPath.h"

namespace
{
    static const char* GetNonEmptyOrNull(const std::string& s)
    {
        return s.empty() ? nullptr : s.c_str();
    }
}

ImGuiProjectLaunchUI::ImGuiProjectLaunchUI(IPlatformSurface *surface, IRenderBackend* renderBackend,
    std::string engineRootPath)
    : m_Surface(surface)
    , m_RenderBackend(renderBackend)
    , m_EngineRootPath(engineRootPath)
{
}

ImGuiProjectLaunchUI::~ImGuiProjectLaunchUI()
{
    Shutdown();
}

bool ImGuiProjectLaunchUI::StartWindow()
{
    if (m_windowInitialized)
        return true;

    // 1) Create window
    FWindowDesc windowDesc;
    windowDesc.width = 1280;
    windowDesc.height = 720;
    windowDesc.title = "JEditor - Project Browser";
    windowDesc.windowState = EWindowState::Normal;

    auto window = m_Surface->CreateWindow(windowDesc);
    if (!window)
        return false;

    m_Window = window;

    m_Surface->MakeContextCurrent(m_Window);

    // 2) Create ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // manual save/load

    // 3) Init backends using the native handle from IPlatformWindow
    void* native = m_Window->GetNativeHandle(); // TODO: Make sure the received window is glfw based, otherwise it will crash
    if (!native)
    {
        std::cerr << "[ImGuiProjectLaunchUI::StartWindow()]: Failed to receive native window." << std::endl;
        return false;
    }

    auto glfwWindow = static_cast<GLFWwindow*>(native);
    ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    m_windowInitialized = true;

    SetupLauncherStyle();
    SetupFonts();

    return true;
}

void ImGuiProjectLaunchUI::ShutdownWindow()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    m_Surface->MakeContextCurrent(nullptr);
    m_Surface->DestroyWindow(m_Window);

    m_windowInitialized = false;
}

void ImGuiProjectLaunchUI::SetupLauncherStyle()
{
    ImGuiTheme::FThemeOptions themeOptions;
    themeOptions.enableDocking = false;
    ImGuiTheme::ApplyEditorTheme(themeOptions);
}

void ImGuiProjectLaunchUI::SetupFonts()
{
    ImGuiTheme::TryLoadDefaultFontFromFile(UPath::Join(m_EngineRootPath, "Assets", "Editor", "Fonts",
        "FunnelSans.ttf"));
}

EProjectLaunchAction ImGuiProjectLaunchUI::PromptForLaunchAction()
{
    if (!StartWindow())
        return EProjectLaunchAction::Cancel;

    // Clear cached results from any previous invocation
    m_CachedProjectToOpen.clear();
    m_CachedCreateRequest = FProjectCreateRequest{};
    m_SelectedItemIndex = -1;
    m_SelectedCategory = EBrowserCategory::RecentProjects;

    RefreshRecentProjects();
    LoadTemplates();

    return RunUnifiedBrowserLoop();
}

EProjectLaunchAction ImGuiProjectLaunchUI::RunUnifiedBrowserLoop()
{
    EProjectLaunchAction chosenAction = EProjectLaunchAction::None;

    bool finished = false;

    while (!finished && !m_Window->ShouldClose())
    {
        // Process OS events
        m_Surface->PollSurfaceEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Fullscreen root window
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);

        ImGuiWindowFlags rootFlags =
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;

        bool rootOpen = true;
        ImGui::Begin("##ProjectBrowserRoot", &rootOpen, rootFlags);

        // Bottom bar height (two rows of controls)
        float bottomBarHeight = ImGui::GetFrameHeight() * 2.1f + 1.0f;

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float mainHeight = avail.y - bottomBarHeight;

        if (mainHeight < 0)
            mainHeight = 0;

        // ================= MAIN REGION =================
        ImGui::BeginChild("MainRegion", ImVec2(0, mainHeight), false);

        if (ImGui::BeginTable("##ProjectBrowserTable", 3,
                              ImGuiTableFlags_BordersInnerV |
                              ImGuiTableFlags_SizingStretchProp,
                              ImVec2(0, 0)))
        {
            ImGui::TableSetupColumn("Sidebar", ImGuiTableColumnFlags_WidthFixed, 220.0f);
            ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthFixed, 360.0f);

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            DrawSidebar();

            ImGui::TableSetColumnIndex(1);
            DrawContentGrid();

            ImGui::TableSetColumnIndex(2);
            DrawDetailsPane();

            ImGui::EndTable();
        }

        ImGui::EndChild();

        // ================= BOTTOM BAR =================
        ImGui::BeginChild("BottomBar", ImVec2(0, 0), false);
        DrawBottomBar(chosenAction, finished);
        ImGui::EndChild();

        // ================= ERROR MODAL =================
        if (m_ShowingError)
        {
            // Request opening the popup this frame
            ImGui::OpenPopup(m_ErrorTitle.c_str());
        }

        // Draw the popup if it's open
        DrawErrorPopup(m_ErrorTitle, m_ErrorMessage);

        // ================= END ROOT WINDOW =================
        ImGui::End(); // Root Window

        // If the user clicked the OS close button (rootOpen becomes false),
        // treat it as Quit.
        if (!rootOpen && !finished)
        {
            chosenAction = EProjectLaunchAction::Cancel;
            finished = true;
        }

        // Render
        ImGui::Render();

        m_RenderBackend->ClearColorDepth(0.10f, 0.10f, 0.10f, 1.0f, false);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Present via platform surface
        m_Surface->Present(m_Window);
    }

    if (chosenAction == EProjectLaunchAction::None)
        chosenAction = EProjectLaunchAction::Cancel;

    return chosenAction;
}

void ImGuiProjectLaunchUI::DrawSidebar()
{
    ImGui::BeginChild("Sidebar", ImVec2(0,0), false);
    ImGui::Dummy(ImVec2(0, 10)); // Padding
    ImGui::Indent(10.0f);

    ImGui::TextDisabled("PROJECTS");
    if (ImGui::Selectable("Recent Projects", m_SelectedCategory == EBrowserCategory::RecentProjects))
    {
        m_SelectedCategory = EBrowserCategory::RecentProjects;
        m_SelectedItemIndex = -1;
    }

    ImGui::Dummy(ImVec2(0, 20));

    ImGui::TextDisabled("CREATE NEW");
    if (ImGui::Selectable("Games", m_SelectedCategory == EBrowserCategory::Games))
    {
        m_SelectedCategory = EBrowserCategory::Games;
        m_SelectedItemIndex = -1;
    }
    if (ImGui::Selectable("Animation & Film", m_SelectedCategory == EBrowserCategory::Animation))
    {
        m_SelectedCategory = EBrowserCategory::Animation;
        m_SelectedItemIndex = -1;
    }

    ImGui::Unindent(10.0f);
    ImGui::EndChild();
}

bool ImGuiProjectLaunchUI::PromptForProjectFile(std::string& outProjectFilePath)
{
    // The engine expects to pop a modal here, but because we already captured it
    // in the unified loop, we just instantly return our cached data.
    if (m_CachedProjectToOpen.empty())
        return false;

    outProjectFilePath = m_CachedProjectToOpen;
    return true;
}

bool ImGuiProjectLaunchUI::PromptForEnginePath(const std::string &projectFilePath, std::string &outEnginePath)
{
    // Try to start / reuse the window
    if (!StartWindow())
    {
        std::cerr << "[ImGuiLaunchUI]: Failed to start window for engine path picker.\n";
        return false;
    }

    // Default suggestion based on m_EngineRootPath
    if (outEnginePath.empty() && !m_EngineRootPath.empty())
    {
        outEnginePath = m_EngineRootPath;
    }

    bool done = false;
    bool accepted = false; // true if user clicked OK with a non-empty path

    while (!done && !m_Window->ShouldClose())
    {
        // Process OS events
        m_Surface->PollSurfaceEvents();

        // New ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Draw the simple picker window; it returns true when user hits OK or Cancel
        if (DrawEnginePathPicker(outEnginePath))
        {
            // If outEnginePath is non-empty, consider it accepted
            accepted = !outEnginePath.empty();
            done = true;
        }

        // Render frame
        ImGui::Render();
        m_RenderBackend->ClearColorDepth(0.10f, 0.10f, 0.10f, 1.0f, false);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        m_Surface->Present(m_Window);
    }

    // If window was closed externally, treat as cancel
    if (m_Window->ShouldClose())
    {
        outEnginePath.clear();
        accepted = false;
    }

    return accepted;
}

bool ImGuiProjectLaunchUI::PromptForNewProject(FProjectCreateRequest& outRequest)
{
    // Instantly return our cached data.
    if (m_CachedCreateRequest.projectName.empty() || m_CachedCreateRequest.parentDirectory.empty())
        return false;

    outRequest = m_CachedCreateRequest;
    return true;
}

void ImGuiProjectLaunchUI::ShowError(const std::string& title, const std::string& message)
{
    if (!StartWindow())
    {
        std::cerr << "[LaunchUI]: Error: " << title << ": " << message << "\n";
        return;
    }

    m_ErrorTitle = title;
    m_ErrorMessage = message;
    m_ShowingError = true;
}

void ImGuiProjectLaunchUI::DrawContentGrid()
{
    ImGui::BeginChild("ContentGrid", ImVec2(0,0), false);
    ImGui::Dummy(ImVec2(0, 10));

    const auto& items = (m_SelectedCategory == EBrowserCategory::RecentProjects) ? m_RecentProjects : m_Templates;

    if (items.empty())
    {
        ImGui::TextDisabled("No items found.");
    }
    else
    {
        // Simple list representation for now (we can make this a true grid of cards later)
        for (int i = 0; i < items.size(); ++i)
        {
            bool isSelected = (m_SelectedItemIndex == i);
            if (ImGui::Selectable(items[i].Name.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0, 40)))
            {
                m_SelectedItemIndex = i;
            }
        }
    }

    ImGui::EndChild();
}

void ImGuiProjectLaunchUI::DrawDetailsPane()
{
    ImGui::BeginChild("DetailsPane", ImVec2(0,0), false);
    ImGui::Dummy(ImVec2(0, 10));
    const auto& items = (m_SelectedCategory == EBrowserCategory::RecentProjects) ? m_RecentProjects : m_Templates;

    if (m_SelectedItemIndex >= 0 && m_SelectedItemIndex < items.size())
    {
        const FBrowserItem& item = items[m_SelectedItemIndex];

        // Title
        ImGui::TextUnformatted(item.Name.c_str());
        ImGui::Separator();

        // Placeholder for a thumbnail image
        ImGui::Dummy(ImVec2(0, 10));
        ImGui::Button("Thumbnail Placeholder", ImVec2(ImGui::GetContentRegionAvail().x, 150));
        ImGui::Dummy(ImVec2(0, 10));

        // Details
        ImGui::TextWrapped("%s", item.Description.c_str());
        ImGui::Dummy(ImVec2(0, 10));
        ImGui::TextDisabled("Path: %s", item.Path.c_str());
    }
    else
    {
        ImGui::TextDisabled("Select an item to view details.");
    }

    ImGui::EndChild();
}

void ImGuiProjectLaunchUI::DrawBottomBar(EProjectLaunchAction& outAction, bool& outFinished)
{
    ImGui::Separator();

    // Bottom bar padding
    ImGui::Dummy(ImVec2(0, 4));
    ImGui::Indent(10.0f);

    float rightAlignOffset = ImGui::GetWindowWidth() - 250.0f;

    if (m_SelectedCategory == EBrowserCategory::RecentProjects)
    {
        // --- RECENT PROJECTS BOTTOM BAR ---
        if (ImGui::Button("Browse...", ImVec2(100, 30)))
        {
            std::string folder = m_Surface->OpenFolderDialog(nullptr);
            if (!folder.empty())
            {
                std::string foundPath;
                if (SearchForProjectFile(folder, foundPath))
                {
                    m_CachedProjectToOpen = foundPath;
                    outAction = EProjectLaunchAction::OpenExisting;
                    outFinished = true;
                }
                else
                {
                    ShowError("Project Not Found", "Could not find a .jproject file in the selected directory.");
                }
            }
        }

        ImGui::SameLine(rightAlignOffset);

        if (ImGui::Button("Cancel", ImVec2(100, 30)))
        {
            outAction = EProjectLaunchAction::Cancel;
            outFinished = true;
        }

        ImGui::SameLine();

        // Only enable "Open" if something is selected
        ImGui::BeginDisabled(m_SelectedItemIndex < 0 || m_SelectedItemIndex >= m_RecentProjects.size());
        if (ImGui::Button("Open", ImVec2(100, 30)))
        {
            m_CachedProjectToOpen = m_RecentProjects[m_SelectedItemIndex].Path;
            outAction = EProjectLaunchAction::OpenExisting;
            outFinished = true;
        }
        ImGui::EndDisabled();
    }
    else
    {
        // --- TEMPLATES / CREATE PROJECT BOTTOM BAR ---
        ImGui::Text("Project Name:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200.0f);
        ImGui::InputText("##ProjName", m_NewProjectName, sizeof(m_NewProjectName));

        ImGui::SameLine();
        ImGui::Text("Location:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(300.0f);
        ImGui::InputText("##ProjPath", m_NewProjectPath, sizeof(m_NewProjectPath));

        ImGui::SameLine();
        if (ImGui::Button("...", ImVec2(30, 0)))
        {
            std::string currentFolder = m_NewProjectPath;
            std::string folder = m_Surface->OpenFolderDialog(GetNonEmptyOrNull(currentFolder));
            if (!folder.empty()) std::snprintf(m_NewProjectPath, sizeof(m_NewProjectPath), "%s", folder.c_str());
        }

        ImGui::SameLine(rightAlignOffset);

        if (ImGui::Button("Cancel", ImVec2(100, 30)))
        {
            outAction = EProjectLaunchAction::Cancel;
            outFinished = true;
        }

        ImGui::SameLine();

        // Ensure name and path are filled
        bool canCreate = (strlen(m_NewProjectName) > 0) && (strlen(m_NewProjectPath) > 0) && (m_SelectedItemIndex >= 0);
        ImGui::BeginDisabled(!canCreate);
        if (ImGui::Button("Create", ImVec2(100, 30)))
        {
            m_CachedCreateRequest.projectName = m_NewProjectName;
            m_CachedCreateRequest.parentDirectory = m_NewProjectPath;
            // TODO: m_CachedCreateRequest.templatePath = m_Templates[m_SelectedItemIndex].Path;
            outAction = EProjectLaunchAction::CreateNew;
            outFinished = true;
        }
        ImGui::EndDisabled();
    }

    ImGui::Unindent(10.0f);
}

bool ImGuiProjectLaunchUI::SearchForProjectFile(const std::filesystem::path& folderPath, std::string& outProjectPath)
{
    try
    {
        for (auto& entry : std::filesystem::recursive_directory_iterator(folderPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".jproject")
            {
                outProjectPath = entry.path().string();
                return true;
            }
        }
    }
    catch (...)
    {
        // Catch filesystem permission errors
    }
    return false;
}

void ImGuiProjectLaunchUI::RefreshRecentProjects()
{
    m_RecentProjects.clear();
    // TODO: MOCK DATA: Replace with actual config file loading later
    m_RecentProjects.push_back({"My Peak Game", "C:/JEngineProjects/MyPeakGame/MyPeakGame.jproject", "Last modified: Today", false});
    m_RecentProjects.push_back({"Test Sandbox", "D:/Work/TestSandbox/TestSandbox.jproject", "Last modified: Yesterday", false});
}

void ImGuiProjectLaunchUI::LoadTemplates()
{
    m_Templates.clear();
    // TODO: MOCK DATA: Replace with scanning the Engine/Templates directory later
    m_Templates.push_back({"Blank Project", "", "A clean empty project with no starter content.", true});
    m_Templates.push_back({"First Person", "", "A project template featuring a first-person character.", true});
    m_Templates.push_back({"Third Person", "", "A project template featuring a third-person character.", true});
}

void ImGuiProjectLaunchUI::Shutdown()
{
    if (m_windowInitialized)
    {
        ShutdownWindow();
    }
}

bool ImGuiProjectLaunchUI::DrawEnginePathPicker(std::string &outEnginePath)
{
    ImGui::SetNextWindowSize(ImVec2(700, 250), ImGuiCond_FirstUseEver);
    ImGui::Begin("Select Engine Executable", nullptr, ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Could not find the engine executable to run this project. Please locate the engine's folder:");
    ImGui::Separator();

    static char pathBuffer[512] = {0};
    static bool initialized = false;

    if (!initialized)
    {
        if (!outEnginePath.empty())
        {
            std::snprintf(pathBuffer, sizeof(pathBuffer), "%s", outEnginePath.c_str());
        }
        initialized = true;
    }

    ImGui::InputText("Engine Root or Executable", pathBuffer, sizeof(pathBuffer));

    if (ImGui::Button("Browse..."))
    {
        std::string currentPath = pathBuffer;
        std::string defaultDir;

        if (!currentPath.empty())
        {
            auto pos = currentPath.find_last_of("/\\");
            if (pos != std::string::npos)
                defaultDir = currentPath.substr(0, pos);
            else
                defaultDir = currentPath;
        }

        const char* defaultPath = GetNonEmptyOrNull(defaultDir);

        // Engine paths are typically folders; use folder dialog
        std::string path = m_Surface->OpenFolderDialog(defaultPath);
        if (!path.empty())
        {
            std::snprintf(pathBuffer, sizeof(pathBuffer), "%s", path.c_str());
        }
    }

    bool done = false;

    if (ImGui::Button("OK"))
    {
        outEnginePath = pathBuffer;
        if (!outEnginePath.empty())
        {
            done = true;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        outEnginePath.clear();
        done = true;
    }

    ImGui::End();
    return done;
}

void ImGuiProjectLaunchUI::DrawErrorPopup(const std::string &title, const std::string &message)
{
    bool open = true;

    // Set a minimum size; the window can grow if needed.
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(400.0f, 0.0f),   // min size (width 400, height auto)
        ImVec2(FLT_MAX, FLT_MAX) // max size (no limit)
    );

    if (ImGui::BeginPopupModal(title.c_str(), &open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextWrapped("%s", message.c_str());

        if (ImGui::Button("OK"))
        {
            ImGui::CloseCurrentPopup();
            open = false;
        }

        ImGui::EndPopup();
    }

    if (!open)
        m_ShowingError = false; // ensure member exists
}
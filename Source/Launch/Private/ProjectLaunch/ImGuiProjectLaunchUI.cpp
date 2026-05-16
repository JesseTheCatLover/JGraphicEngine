//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "ProjectLaunch/ImGuiProjectLaunchUI.h"

#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Rendering/IRenderBackend.h"
#include "Rendering/IPlatformSurface.h"
#include "Rendering/IPlatformWindow.h"

namespace
{
    static const char* GetNonEmptyOrNull(const std::string& s)
    {
        return s.empty() ? nullptr : s.c_str();
    }
}

ImGuiProjectLaunchUI::ImGuiProjectLaunchUI(IPlatformSurface *surface, IRenderBackend* renderBackend)
    : m_Surface(surface)
    , m_RenderBackend(renderBackend)
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
    ImGui::StyleColorsDark();

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

EProjectLaunchAction ImGuiProjectLaunchUI::PromptForLaunchAction()
{
    if (!StartWindow())
        return EProjectLaunchAction::Cancel;

    EProjectLaunchAction chosenAction = EProjectLaunchAction::Cancel;

    auto draw = [&](bool& accepted, bool& running) -> bool
    {
        ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_FirstUseEver);
        ImGui::Begin("JEditor Project Browser", nullptr, ImGuiWindowFlags_NoCollapse);
        ImGui::Text("Welcome to JEditor");
        ImGui::Separator();

        if (ImGui::Button("Open Existing Project", ImVec2(200, 0)))
        {
            chosenAction = EProjectLaunchAction::OpenExisting;
            accepted = true;
            ImGui::End();
            return true; // done
        }

        if (ImGui::Button("Create New Project", ImVec2(200, 0)))
        {
            chosenAction = EProjectLaunchAction::CreateNew;
            accepted = true;
            ImGui::End();
            return true; // done
        }

        if (ImGui::Button("Cancel", ImVec2(200, 0)))
        {
            chosenAction = EProjectLaunchAction::Cancel;
            accepted = false;
            ImGui::End();
            return true; // done
        }

        ImGui::End();
        return false; // keep looping
    };

    bool ok = RunModalLoop(draw);
    if (!ok && chosenAction != EProjectLaunchAction::OpenExisting &&
        chosenAction != EProjectLaunchAction::CreateNew)
    {
        chosenAction = EProjectLaunchAction::Cancel;
    }

    return chosenAction;
}

bool ImGuiProjectLaunchUI::PromptForProjectFile(std::string& outProjectFilePath)
{
    if (!StartWindow())
        return false;

    outProjectFilePath.clear();

    auto draw = [&](bool& accepted, bool& running) -> bool
    {
        bool done = DrawProjectFilePicker(outProjectFilePath);
        if (done)
        {
            accepted = !outProjectFilePath.empty();
            running  = false;
        }
        return done;
    };

    return RunModalLoop(draw);
}

bool ImGuiProjectLaunchUI::PromptForEnginePath(const std::string& projectFilePath, std::string& outEnginePath)
{
    if (!StartWindow())
        return false;

    outEnginePath.clear();

    auto draw = [&](bool& accepted, bool& running) -> bool
    {
        bool done = DrawEnginePathPicker(projectFilePath, outEnginePath);
        if (done)
        {
            accepted = !outEnginePath.empty();
            running  = false;
        }
        return done;
    };

    return RunModalLoop(draw);
}

bool ImGuiProjectLaunchUI::PromptForNewProject(FProjectCreateRequest& outRequest)
{
    if (!StartWindow())
        return false;

    outRequest = {}; // reset

    auto draw = [&](bool& accepted, bool& running) -> bool
    {
        bool done = DrawNewProjectScreen(outRequest);
        if (done)
        {
            // For now, accept if name and folder are non-empty
            accepted = !outRequest.projectName.empty() &&
                       !outRequest.parentDirectory.empty();
            running  = false;
        }
        return done;
    };

    return RunModalLoop(draw);
}

void ImGuiProjectLaunchUI::ShowError(const std::string& title, const std::string& message)
{
    if (!StartWindow())
    {
        // fallback
        std::cerr << "[LaunchUI Error] " << title << ": " << message << "\n";
        return;
    }

    ImGui::OpenPopup(title.c_str());

    auto draw = [&](bool& accepted, bool& running) -> bool
    {
        DrawErrorPopup(title, message);
        // We exit when popup closes
        bool popupStillOpen = ImGui::IsPopupOpen(title.c_str());
        if (!popupStillOpen)
        {
            accepted = true;  // doesn't matter much for errors
            running  = false;
            return true;
        }
        return false;
    };

    RunModalLoop(draw);
}

void ImGuiProjectLaunchUI::Shutdown()
{
    if (m_windowInitialized)
    {
        ShutdownWindow();
    }
}

template<typename DrawFunc>
bool ImGuiProjectLaunchUI::RunModalLoop(DrawFunc drawFunc)
{
    if (!m_Window)
        return false;

    bool running = true;
    bool accepted = false;

    while (running)
    {
        m_Surface->PollSurfaceEvents();

        if (m_Window->ShouldClose())
        {
            running = false;
            accepted = false;
            break;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        m_RenderBackend->ClearColorDepth(0.f, 0.f, 0.f, 1.f);

        // Let the given draw function render its UI and optionally tell us when to accept/cancel.
        bool done = drawFunc(accepted, running);
        if (done)
        {
            running = false;
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        m_Surface->SwapBuffers(m_Window);
    }

    return accepted;
}

bool ImGuiProjectLaunchUI::DrawLaunchActionScreen(EProjectLaunchAction &outAction)
{
    ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("JEditor Project Browser", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::Text("Welcome to JEditor");
    ImGui::Separator();

    bool done = false;

    if (ImGui::Button("Open Existing Project", ImVec2(200, 0)))
    {
        outAction = EProjectLaunchAction::OpenExisting;
        done = true;
    }

    if (ImGui::Button("Create New Project", ImVec2(200, 0)))
    {
        outAction = EProjectLaunchAction::CreateNew;
        done = true;
    }

    if (ImGui::Button("Cancel", ImVec2(200, 0)))
    {
        outAction = EProjectLaunchAction::Cancel;
        done = true;
    }

    ImGui::End();
    return done; // true => caller should exit modal loop
}

bool ImGuiProjectLaunchUI::DrawProjectFilePicker(std::string &outProjectFilePath)
{
    ImGui::SetNextWindowSize(ImVec2(700, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Open Project", nullptr, ImGuiWindowFlags_NoCollapse);

    // Static state for this UI
    static char pathBuffer[512] = {0};
    static bool initialized = false;

    // Initialize buffer from current outProjectFilePath once
    if (!initialized)
    {
        if (!outProjectFilePath.empty())
        {
            std::snprintf(pathBuffer, sizeof(pathBuffer), "%s", outProjectFilePath.c_str());
        }
        initialized = true;
    }

    ImGui::InputText("Project File (.jproject)", pathBuffer, sizeof(pathBuffer));

    if (ImGui::Button("Browse..."))
    {
        // Derive default directory from current buffer if possible
        std::string currentPath = pathBuffer;
        std::string defaultDir;

        if (!currentPath.empty())
        {
            // crude: everything up to last slash
            auto pos = currentPath.find_last_of("/\\");
            if (pos != std::string::npos)
                defaultDir = currentPath.substr(0, pos);
        }

        const char* defaultPath = GetNonEmptyOrNull(defaultDir);

        std::string path = m_Surface->OpenFileDialog("jproject", defaultPath);
        if (!path.empty())
        {
            std::snprintf(pathBuffer, sizeof(pathBuffer), "%s", path.c_str());
        }
    }

    bool done = false;

    if (ImGui::Button("OK"))
    {
        outProjectFilePath = pathBuffer;
        if (!outProjectFilePath.empty())
        {
            done = true;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        outProjectFilePath.clear();
        done = true;
    }

    ImGui::End();
    return done;
}

bool ImGuiProjectLaunchUI::DrawEnginePathPicker(const std::string &projectFilePath, std::string &outEnginePath)
{
    ImGui::SetNextWindowSize(ImVec2(700, 250), ImGuiCond_FirstUseEver);
    ImGui::Begin("Select Engine", nullptr, ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Project '%s' needs an engine path.", projectFilePath.c_str());
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

bool ImGuiProjectLaunchUI::DrawNewProjectScreen(FProjectCreateRequest &outRequest)
{
    ImGui::SetNextWindowSize(ImVec2(700, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Create New Project", nullptr, ImGuiWindowFlags_NoCollapse);

    static char nameBuffer[128] = {0};
    static char folderBuffer[512] = {0};
    static bool initialized = false;

    if (!initialized)
    {
        if (!outRequest.projectName.empty())
            std::snprintf(nameBuffer, sizeof(nameBuffer), "%s", outRequest.projectName.c_str());
        if (!outRequest.parentDirectory.empty())
            std::snprintf(folderBuffer, sizeof(folderBuffer), "%s", outRequest.parentDirectory.c_str());
        initialized = true;
    }

    ImGui::InputText("Project Name", nameBuffer, sizeof(nameBuffer));
    ImGui::InputText("Project Folder", folderBuffer, sizeof(folderBuffer));

    if (ImGui::Button("Browse..."))
    {
        std::string currentFolder = folderBuffer;
        const char* defaultPath = GetNonEmptyOrNull(currentFolder);

        std::string chosenFolder = m_Surface->OpenFolderDialog(defaultPath);
        if (!chosenFolder.empty())
        {
            std::snprintf(folderBuffer, sizeof(folderBuffer), "%s", chosenFolder.c_str());
        }
    }

    bool done = false;

    if (ImGui::Button("Create"))
    {
        std::string name   = nameBuffer;
        std::string folder = folderBuffer;

        if (!name.empty() && !folder.empty())
        {
            outRequest.projectName     = name;
            outRequest.parentDirectory = folder;
            // TODO: set additional fields on outRequest as needed (templates, etc.)
            done = true;
        }
        // Optionally show validation errors here (e.g., ImGui::TextColored)
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        done = true;
    }

    ImGui::End();
    return done;
}

void ImGuiProjectLaunchUI::DrawErrorPopup(const std::string &title, const std::string &message)
{
    bool open = true;

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
}
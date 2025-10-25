#include "EditorApp.h"

#include <Core/CoreMinimal.h>
#include <GLFW/glfw3.h>
#include <EditorContext.h>
#include <ImGuiLayer.h>
#include <iostream>

#include <Panels/SceneHierarchyPanel.h>

EditorApp::EditorApp()
{
}

EditorApp::~EditorApp() = default;

void EditorApp::BeginFrame()
{
    m_ImGuiLayer->BeginFrame();
}

void EditorApp::RenderPanels()
{
    if (JEngine::Get().GetState().GetViewMode() != EViewMode::UI) return;
    m_SceneHierarchyPanel->Draw(*m_Context);
}

void EditorApp::EndFrame()
{
    m_ImGuiLayer->EndFrame();
}

void EditorApp::Shutdown()
{
    m_ImGuiLayer->Shutdown();
}

void EditorApp::OnEngineInitialized(GLFWwindow* window)
{
    if (!window)
    {
        std::cerr << "[EditorApp]: Failed to initialize, Given window is null" << std::endl;
        return;
    }
    m_Window = window;

    m_ImGuiLayer = std::make_unique<ImGuiLayer>(window);
    m_Context = std::make_unique<EditorContext>(GEngine->GetState());
    m_SceneHierarchyPanel = std::make_unique<SceneHierarchyPanel>();
}

void EditorApp::OnSceneLoaded(const std::string &sceneName)
{
}

void EditorApp::OnRenderOverlay()
{
    EndFrame();
}

void EditorApp::OnTick(float deltaTime)
{
    BeginFrame();
    RenderPanels();
}

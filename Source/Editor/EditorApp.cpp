//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "EditorApp.h"

#include <iostream>

#include "Engine/JEngine.h"

EditorApp::EditorApp(GLFWwindow *window):
    m_ImGuiLayer(window),
    m_Context(JEngine::Get().GetState())
{}

void EditorApp::BeginFrame()
{
    m_ImGuiLayer.BeginFrame();
}

void EditorApp::RenderPanels()
{
    m_SceneHierarchyPanel.Draw(m_Context);
}

void EditorApp::EndFrame()
{
    m_ImGuiLayer.EndFrame();
}

void EditorApp::Shutdown()
{
    m_ImGuiLayer.Shutdown();
}

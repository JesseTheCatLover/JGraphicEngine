//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/EngineState.h"

#include "Core/Contexts/FInputContext.h"
#include "Core/Contexts/FViewportContext.h"
#include "Core/Contexts/FFrameContext.h"
#include "Core/Contexts/FWindowContext.h"
#include "GLFW/glfw3.h"

EngineState::EngineState()
{
    m_FrameContext = std::make_unique<FFrameContext>();
    // Initialize LastFrameTime
    m_FrameContext->LastFrameTime = static_cast<float>(glfwGetTime());

    m_WindowContext = std::make_unique<FWindowContext>();
    if (m_WindowContext->bFullscreen)
    {
        m_WindowContext->SetFullscreenSize();
    }

    m_ViewportContext = std::make_unique<FViewportContext>();

}

float EngineState::GetDeltaTime() const
{
    return m_FrameContext->DeltaTime;
}

void EngineState::SetDeltaTime(float dt)
{
    m_FrameContext->DeltaTime = dt;
}

int EngineState::GetWindowWidth() const
{
    return m_WindowContext->width;
}

void EngineState::SetWindowWidth(int w)
{
    m_WindowContext->width = w;
}

int EngineState::GetWindowHeight() const
{
    return m_WindowContext->height;
}

void EngineState::SetWindowHeight(int h)
{
    m_WindowContext->height = h;
}

bool EngineState::GetWireframeMode()
{
    return m_ViewportContext->bWireframe;
}

void EngineState::SetWireframeMode(bool bWireMode)
{
    m_ViewportContext->bWireframe = bWireMode;
}

EViewMode EngineState::GetViewMode() const
{
    return m_ViewportContext->viewMode;
}

void EngineState::SetViewMode(EViewMode mode)
{
    m_ViewportContext->viewMode = mode;
}

float EngineState::GetLastFrameTime() const
{
    return m_FrameContext->LastFrameTime;
}

void EngineState::SetLastFrameTime(float lft)
{
    m_FrameContext->LastFrameTime = lft;
}

GLFWwindow * EngineState::GetGLFWWindow() const
{
    return m_WindowContext->window;
}

void EngineState::SetGLFWWindow(GLFWwindow *window)
{
    m_WindowContext->window = window;
}

bool EngineState::GetIsFirstMouse()
{
    return m_InputContext->bFirstMouse;
}

void EngineState::SetIsFirstMouse(bool bIsFirst)
{
    m_InputContext->bFirstMouse = bIsFirst;
}

float EngineState::GetLastMouseX()
{
    return m_InputContext->lastMouseX;
}

void EngineState::SetLastMouseX(float x)
{
    m_InputContext->lastMouseX = x;
}

float EngineState::GetLastMouseY()
{
    return m_InputContext->lastMouseY;
}

void EngineState::SetLastMouseY(float y)
{
    m_InputContext->lastMouseY = y;
}

JCamera* EngineState::GetCamera() const
{
    return &m_ViewportContext->camera;
}

void EngineState::SetCamera(JCamera *camera)
{
    m_ViewportContext->camera = *camera;
}

FViewportContext * EngineState::GetCameraSettings() const
{
    return m_ViewportContext.get();
}
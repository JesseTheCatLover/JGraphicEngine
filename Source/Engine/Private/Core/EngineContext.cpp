//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "EngineContext.h"
#include "Core/Contexts/FInputContext.h"
#include "Contexts/FViewportContext.h"
#include "Core/Contexts/FFrameContext.h"
#include "Core/Contexts/FSurfaceContext.h"
#include "GLFW/glfw3.h"

EngineContext::EngineContext()
{
    m_FrameContext = MakeUnique<FFrameContext>();
    // Initialize LastFrameTime
    m_FrameContext->LastFrameTime = static_cast<float>(glfwGetTime());

    m_SurfaceContext = MakeUnique<FSurfaceContext>();

    m_ViewportContext = MakeUnique<FViewportContext>();

    m_InputContext = MakeUnique<FInputContext>();

}

EngineContext::~EngineContext() = default;

const float& EngineContext::GetDeltaTime() const
{
    return m_FrameContext->DeltaTime;
}

void EngineContext::SetDeltaTime(float dt)
{
    m_FrameContext->DeltaTime = dt;
}

int EngineContext::GetFramebufferWidth() const
{
    return m_SurfaceContext->width;
}

void EngineContext::SetFramebufferWidth(int w)
{
    m_SurfaceContext->width = w;
}

int EngineContext::GetFramebufferHeight() const
{
    return m_SurfaceContext->height;
}

void EngineContext::SetFramebufferHeight(int h)
{
    m_SurfaceContext->height = h;
}

float EngineContext::GetAspectRatio() const
{
    return m_SurfaceContext->aspectRatio;
}

bool EngineContext::GetIsSurfaceFullscreen()
{
    return m_SurfaceContext->bFullscreen;
}

bool EngineContext::GetWireframeMode()
{
    return m_ViewportContext->bWireframe;
}

void EngineContext::SetWireframeMode(bool bWireMode)
{
    m_ViewportContext->bWireframe = bWireMode;
}

float EngineContext::GetLastFrameTime() const
{
    return m_FrameContext->LastFrameTime;
}

void EngineContext::SetLastFrameTime(float lft)
{
    m_FrameContext->LastFrameTime = lft;
}

bool EngineContext::GetIsFirstMouse()
{
    return m_InputContext->bFirstMouse;
}

void EngineContext::SetIsFirstMouse(bool bIsFirst)
{
    m_InputContext->bFirstMouse = bIsFirst;
}

float EngineContext::GetLastMouseX()
{
    return m_InputContext->lastMouseX;
}

void EngineContext::SetLastMouseX(float x)
{
    m_InputContext->lastMouseX = x;
}

float EngineContext::GetLastMouseY()
{
    return m_InputContext->lastMouseY;
}

void EngineContext::SetLastMouseY(float y)
{
    m_InputContext->lastMouseY = y;
}

ICameraViewSource* EngineContext::GetCamera() const
{
    return m_ViewportContext->camera;
}

void EngineContext::SetCamera(ICameraViewSource *camera)
{
    m_ViewportContext->camera = camera;
}

FViewportContext* EngineContext::GetCameraSettings() const
{
    return m_ViewportContext.get();
}
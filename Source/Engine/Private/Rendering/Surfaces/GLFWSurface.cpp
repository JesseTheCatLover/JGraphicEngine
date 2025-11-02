// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "GLFWSurface.h"
#include "GLFW/glfw3.h"
#include <iostream>

GLFWSurface::~GLFWSurface()
{
    GLFWSurface::Shutdown();
}

bool GLFWSurface::Initialize(const FSurfaceState &state)
{
    if (!glfwInit())
    {
        std::cerr << "[GLFWSurface]: Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    m_State = state;

    GLFWmonitor* monitor = static_cast<GLFWmonitor*>(state.monitorHandle);
    if (!monitor)
        monitor = glfwGetPrimaryMonitor(); // default fallback if not specified

    if (IsFullscreen() || m_State.windowState == EWindowState::Maximized)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        m_State.width = mode->width;
        m_State.height = mode->height;
    }
    else // Windowed (with borders) in saved width/height
    {
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);  // title bar
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
        glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
        glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);   // stays behind other floating windows
        m_State.width = state.width;
        m_State.height = state.height;
    }

    GLFWmonitor* winMonitor = IsFullscreen() ? monitor : nullptr;
    m_Window = glfwCreateWindow(m_State.width, m_State.height, state.title.c_str(),  winMonitor, nullptr);

    if (!m_Window)
    {
        std::cerr << "[GLFWSurface]: Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    // Store the native handle
    m_State.nativeHandle = m_Window;
    m_State.monitorHandle = monitor;

    // Make context current (for OpenGL)
    glfwMakeContextCurrent(m_Window); // TODO: Check only if using OpenGL with a macro or smth else.

    // Apply vsync
    glfwSwapInterval(state.bvSync ? 1 : 0);

    return true;
}

void GLFWSurface::Shutdown()
{
    if (m_Window)
    {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }

    glfwTerminate();
    std::cout << "[GLFWSurface]: Shutdown completed" << std::endl;
}

IPlatformSurface::GetProcAddressFunc GLFWSurface::GetProcAddressFunction() const
{
    // Explicitly cast the GLFW function to our expected signature.
    return reinterpret_cast<GetProcAddressFunc>(glfwGetProcAddress);
}

void GLFWSurface::Present()
{
    SwapBuffers();
}

void GLFWSurface::SwapBuffers()
{
    if (m_Window)
        glfwSwapBuffers(m_Window);
}

void GLFWSurface::Resize(int width, int height)
{
    m_State.width = width;
    m_State.height = height;
    glfwSetWindowSize(m_Window, width, height);
}

void GLFWSurface::PollSurfaceEvents()
{
    glfwPollEvents();
}

void * GLFWSurface::GetNativeHandle() const
{
    return reinterpret_cast<void*>(m_Window);
}

bool GLFWSurface::IsFullscreen() const
{
    return m_State.windowState == EWindowState::Fullscreen ? true : false;
}

int GLFWSurface::GetWidth() const
{
    return m_State.width;
}

int GLFWSurface::GetHeight() const
{
    return m_State.height;
}

void GLFWSurface::SetCursorMode(ECursorMode mode)
{
    m_CursorMode = mode;
    UpdateCursor();
}

void GLFWSurface::GetFramebufferSize(int &w, int &h) const
{
    if (m_Window) glfwGetFramebufferSize(m_Window, &w, &h);
    else { w = h = 0; }
}

void GLFWSurface::SetCursorVisible()
{
    m_CursorMode = ECursorMode::Visible;
    UpdateCursor();
}

void GLFWSurface::SetCursorHidden()
{
    m_CursorMode = ECursorMode::Hidden;
    UpdateCursor();
}

void GLFWSurface::SetCursorDisabled()
{
    m_CursorMode = ECursorMode::Disabled;
    UpdateCursor();
}

bool GLFWSurface::IsVSyncEnabled() const
{
    return m_State.bvSync;
}

FSurfaceState GLFWSurface::GetState() const
{
    return m_State;
}

void GLFWSurface::SetTitle(const std::string &title)
{
    m_State.title = title;
    if (m_Window)
        glfwSetWindowTitle(m_Window, title.c_str());
}

void GLFWSurface::SetVSync(bool vSync)
{
    m_State.bvSync = vSync;
    if (m_Window) glfwSwapInterval(vSync ? 1 : 0);
}

void GLFWSurface::UpdateCursor()
{
    if (!m_Window) return;
    switch (m_CursorMode)
    {
        case ECursorMode::Visible:
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            break;
        case ECursorMode::Hidden:
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            break;
        case ECursorMode::Disabled:
            // Capture and hide the cursor
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            break;
    }
}

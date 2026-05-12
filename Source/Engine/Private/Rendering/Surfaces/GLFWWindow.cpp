//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "GLFWWindow.h"

#include "GLFW/glfw3.h"
#include <iostream>

GLFWWindow::GLFWWindow(const FWindowDesc& initialState, GLFWwindow* shareContext)
    : m_State(initialState)
    , m_ShareContext(shareContext)
{
}

GLFWWindow::~GLFWWindow()
{
}

bool GLFWWindow::Initialize()
{
    // GLFW must already be initialized by GLFWSurface::Initialize()

    GLFWmonitor* monitor = static_cast<GLFWmonitor*>(m_State.monitorHandle);
    if (!monitor)
        monitor = glfwGetPrimaryMonitor();

    if (IsFullscreen() || m_State.windowState == EWindowState::Maximized)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwWindowHint(GLFW_RED_BITS,     mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS,   mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS,    mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

        m_State.width  = mode->width;
        m_State.height = mode->height;
    }
    else
    {
        // windowed
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
        glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
        glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);
        // keep width/height from m_State as passed in
    }

    GLFWmonitor* winMonitor = IsFullscreen() ? monitor : nullptr;
    m_Window = glfwCreateWindow(
        m_State.width,
        m_State.height,
        m_State.title.c_str(),
        winMonitor,
        m_ShareContext
    );

    if (!m_Window)
    {
        std::cerr << "[GLFWWindow]: Failed to create GLFW window\n";
        return false;
    }

    m_State.nativeHandle  = m_Window;
    m_State.monitorHandle = monitor;

    glfwSetWindowUserPointer(m_Window, this);

    // Framebuffer resize callback
    glfwSetFramebufferSizeCallback(
        m_Window,
        [](GLFWwindow* window, int width, int height)
        {
            auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            if (!self) return;

            if (self->m_FramebufferResizeCallback)
                self->m_FramebufferResizeCallback(width, height);
        });

    // Window size callback
    glfwSetWindowSizeCallback(
        m_Window,
        [](GLFWwindow* window, int width, int height)
        {
            auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            if (!self) return;

            // update cached size
            self->m_State.width  = width;
            self->m_State.height = height;

            if (self->m_WindowResizeCallback)
                self->m_WindowResizeCallback(width, height);
        });

    // Window focus callback
    glfwSetWindowFocusCallback(
    m_Window,
    [](GLFWwindow* window, int focused)
    {
        auto* self =
            static_cast<GLFWWindow*>(
                glfwGetWindowUserPointer(window));

        if (!self)
            return;

        if (self->m_FocusCallback)
        {
            self->m_FocusCallback(
                *self,
                focused == GLFW_TRUE);
        }
    });

    // Window close callback
    glfwSetWindowCloseCallback(
        m_Window,
        [](GLFWwindow* window)
        {
            auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            if (!self) return;

            // This will call the close handler, which will call surface->DestroyWindow(...)
            self->SetShouldClose(true);
        });

    // Apply vsync
    glfwSwapInterval(m_State.bvSync ? 1 : 0);

    UpdateCursor();
    return true;
}

void GLFWWindow::Shutdown()
{
    if (m_Window)
    {
        // Clear the user pointer so any future GLFW callbacks become harmless
        glfwSetWindowUserPointer(m_Window, nullptr);

        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
}

void GLFWWindow::SetCloseHandler(FCloseHandler handler)
{
    m_CloseHandler = std::move(handler);
}

void GLFWWindow::SetSurfaceSize(int width, int height)
{
    m_State.width  = width;
    m_State.height = height;

    if (m_Window)
        glfwSetWindowSize(m_Window, width, height);
}

void GLFWWindow::GetWindowSize(int& w, int& h) const
{
    if (m_Window)
    {
        glfwGetWindowSize(m_Window, &w, &h);
    }
    else
    {
        w = 0;
        h = 0;
    }
}

void GLFWWindow::GetFramebufferSize(int& w, int& h) const
{
    if (m_Window)
    {
        glfwGetFramebufferSize(m_Window, &w, &h);
    }
    else
    {
        w = 0;
        h = 0;
    }
}

bool GLFWWindow::ShouldClose() const
{
    return m_Window ? glfwWindowShouldClose(m_Window) == GLFW_TRUE : true;
}

void GLFWWindow::SetShouldClose(bool bShould)
{
    if (!m_Window)
        return;

    // Let GLFW know for consistency (e.g., ShouldClose() checks, etc.)
    glfwSetWindowShouldClose(m_Window, bShould ? GLFW_TRUE : GLFW_FALSE);

    if (bShould && m_CloseHandler)
    {
        // IMPORTANT: do NOT destroy the window here.
        // Just notify the surface via the handler. The surface will call Shutdown() and erase from its list.
        m_CloseHandler(*this);
    }
}

int GLFWWindow::GetWidth() const
{
    return m_State.width;
}

int GLFWWindow::GetHeight() const
{
    return m_State.height;
}

float GLFWWindow::GetAspectRatio() const
{
    if (m_State.height <= 0 || m_State.width <= 0)
        return 1.0f;
    return static_cast<float>(m_State.width) / static_cast<float>(m_State.height);
}

bool GLFWWindow::IsVSyncEnabled() const
{
    return m_State.bvSync;
}

bool GLFWWindow::IsFullscreen() const
{
    return m_State.windowState == EWindowState::Fullscreen;
}

FWindowDesc GLFWWindow::GetState() const
{
    return m_State;
}

void GLFWWindow::SetTitle(const std::string& title)
{
    m_State.title = title;
    if (m_Window)
        glfwSetWindowTitle(m_Window, title.c_str());
}

void GLFWWindow::SetVSync(bool vSync)
{
    m_State.bvSync = vSync;
    if (m_Window)
        glfwSwapInterval(vSync ? 1 : 0);
}

void* GLFWWindow::GetNativeHandle() const
{
    return reinterpret_cast<void*>(m_Window);
}

void GLFWWindow::SetCursorMode(ECursorMode mode)
{
    m_CursorMode = mode;
    UpdateCursor();
}

void GLFWWindow::SetCursorVisible()
{
    m_CursorMode = ECursorMode::Visible;
    UpdateCursor();
}

void GLFWWindow::SetCursorHidden()
{
    m_CursorMode = ECursorMode::Hidden;
    UpdateCursor();
}

void GLFWWindow::SetCursorDisabled()
{
    m_CursorMode = ECursorMode::Disabled;
    UpdateCursor();
}

void GLFWWindow::SetFramebufferResizeCallback(FResizeCallback callback)
{
    m_FramebufferResizeCallback = std::move(callback);
}

void GLFWWindow::SetWindowResizeCallback(FResizeCallback callback)
{
    m_WindowResizeCallback = std::move(callback);
}

void GLFWWindow::SetFocusCallback(FFocusCallback callback)
{
    m_FocusCallback = std::move(callback);
}

void GLFWWindow::UpdateCursor()
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
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            break;
    }
}

// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "GLFWSurface.h"
#include "GLFW/glfw3.h"
#include "nfd.h"

#include <iostream>
#include <sstream>

namespace
{
    std::vector<nfdnfilteritem_t> BuildNfdFilter(const char* filterList,
                                                 std::vector<std::string>& backingStrings)
    {
        std::vector<nfdnfilteritem_t> result;
        if (!filterList || *filterList == '\0')
            return result;

        // We'll store name/spec strings in backingStrings so their memory stays valid.
        std::stringstream ss(filterList);
        std::string group;
        while (std::getline(ss, group, ';'))
        {
            if (group.empty())
                continue;

            std::string name;
            std::string spec;

            auto colonPos = group.find(':');
            if (colonPos != std::string::npos)
            {
                name = group.substr(0, colonPos);
                spec = group.substr(colonPos + 1);
            }
            else
            {
                // If no name, use spec as both name and spec.
                name = group;
                spec = group;
            }

            // Store in backingStrings so c_str() is stable
            backingStrings.push_back(name);
            backingStrings.push_back(spec);

            nfdnfilteritem_t item{};
            item.name = backingStrings[backingStrings.size() - 2].c_str();
            item.spec = backingStrings[backingStrings.size() - 1].c_str();
            result.push_back(item);
        }

        return result;
    }
}

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

    // Make glfw allow binding to c++ objects
    glfwSetWindowUserPointer(m_Window, this);

    // Framebuffer callback (often fires at end / on DPI changes)
    glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
    {
        auto* surface = static_cast<GLFWSurface*>(glfwGetWindowUserPointer(window));
        if (!surface)
            return;

        if (surface->m_FramebufferResizeCallback)
            surface->m_FramebufferResizeCallback(width, height);
    });

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

void GLFWSurface::SetSurfaceSize(int width, int height)
{
    m_State.width = width;
    m_State.height = height;
    glfwSetWindowSize(m_Window, width, height);
}

void GLFWSurface::PollSurfaceEvents()
{
    glfwPollEvents();
}

bool GLFWSurface::ShouldClose() const
{
    return glfwWindowShouldClose(m_Window);
}

void GLFWSurface::SetShouldClose(bool bShould)
{
    glfwSetWindowShouldClose(m_Window, bShould);
}

void GLFWSurface::GetWindowSize(int &w, int &h) const
{
    if (m_Window)
        glfwGetWindowSize(m_Window, &w, &h);
    else
    {
        w = 0;
        h = 0;
    }
}

void* GLFWSurface::GetNativeHandle() const
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

float GLFWSurface::GetAspectRatio() const
{
    if (m_State.height <= 0 || m_State.width <= 0)
        return 1.0f;
    return static_cast<float>(m_State.width) / static_cast<float>(m_State.height);
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

float GLFWSurface::GetTimeSeconds()
{
    return static_cast<float>(glfwGetTime());
}

std::string GLFWSurface::OpenFileDialog(const char* filterList, const char* defaultPath)
{
    nfdnchar_t* outPath = nullptr;

    std::vector<std::string> filterBacking;
    auto filters = BuildNfdFilter(filterList, filterBacking);
    const nfdnfilteritem_t* filterPtr = filters.empty() ? nullptr : filters.data();
    nfdfiltersize_t filterCount = static_cast<nfdfiltersize_t>(filters.size());

    nfdresult_t result = NFD_OpenDialogN(
        &outPath,       // outPath
        filterPtr,      // filterList
        filterCount,    // filterCount
        defaultPath     // defaultPath
    );

    std::string path;
    if (result == NFD_OKAY && outPath)
    {
        path = outPath;
        NFD_FreePathN(outPath);
    }

    return path;
}

std::vector<std::string> GLFWSurface::OpenFileDialogMultiple(const char* filterList, const char* defaultPath)
{
    const nfdpathset_t* outPaths = nullptr;

    std::vector<std::string> filterBacking;
    auto filters = BuildNfdFilter(filterList, filterBacking);
    const nfdnfilteritem_t* filterPtr = filters.empty() ? nullptr : filters.data();
    nfdfiltersize_t filterCount = static_cast<nfdfiltersize_t>(filters.size());

    nfdresult_t result = NFD_OpenDialogMultipleN(
        &outPaths,      // outPaths
        filterPtr,      // filterList
        filterCount,    // filterCount
        defaultPath     // defaultPath
    );

    std::vector<std::string> paths;
    if (result == NFD_OKAY && outPaths)
    {
        // 1) Get count via out parameter
        nfdpathsetsize_t count = 0;
        nfdresult_t countResult = NFD_PathSet_GetCount(outPaths, &count);
        if (countResult == NFD_OKAY && count > 0)
        {
            paths.reserve(static_cast<size_t>(count));

            // 2) Iterate and get each path via out parameter
            for (nfdpathsetsize_t i = 0; i < count; ++i)
            {
                nfdnchar_t* p = nullptr;
                nfdresult_t pathResult = NFD_PathSet_GetPathN(outPaths, i, &p);
                if (pathResult == NFD_OKAY && p)
                    paths.emplace_back(p);
            }
        }

        // 3) Free the path set
        NFD_PathSet_Free(outPaths);
    }

    return paths;
}

std::string GLFWSurface::OpenFolderDialog(const char* defaultPath)
{
    nfdnchar_t* outPath = nullptr;

    nfdresult_t result = NFD_PickFolderN(
        &outPath,       // outPath
        defaultPath     // defaultPath
    );

    std::string path;
    if (result == NFD_OKAY && outPath)
    {
        path = outPath;
        NFD_FreePathN(outPath);
    }

    return path;
}

std::string GLFWSurface::SaveFileDialog(const char* filterList, const char* defaultPath, const char* defaultName)
{
    nfdchar_t* outPath = nullptr;

    std::vector<std::string> filterBacking;
    auto filters = BuildNfdFilter(filterList, filterBacking);
    const nfdfilteritem_t* filterPtr = filters.empty() ? nullptr : filters.data();
    nfdfiltersize_t filterCount = static_cast<nfdfiltersize_t>(filters.size());


    nfdresult_t result = NFD_SaveDialogN(
        &outPath,           // outPath
        filterPtr,          // filterList
        filterCount,        // filterCount
        defaultPath,        // defaultPath
        defaultName         // defaultName
    );

    std::string path;
    if (result == NFD_OKAY && outPath)
    {
        path = outPath;
        NFD_FreePathN(outPath);
    }

    return path;
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

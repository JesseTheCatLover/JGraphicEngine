// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "GLFWSurface.h"
#include "GLFW/glfw3.h"
#include "nfd.h"

#include <iostream>
#include <sstream>

#include "GLFWWindow.h"

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
    Shutdown();
}

bool GLFWSurface::Initialize()
{
    if (m_Initialized)
        return true;

    if (!glfwInit())
    {
        std::cerr << "[GLFWSurface]: Failed to initialize GLFW" << std::endl;
        return false;
    }

    m_Initialized = true;
    m_Shutdown = false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    return true;
}

void GLFWSurface::Shutdown()
{
    if (m_Shutdown)
        return;

    m_Shutdown = true;

    // Destroy windows first
    for (auto& window : m_Windows)
    {
        if (window)
            window->Shutdown();
    }

    m_Windows.clear();
    m_PrimaryWindow.reset();

    glfwTerminate();
    std::cout << "[GLFWSurface]: Shutdown completed\n";
}

TSharedPtr<IPlatformWindow> GLFWSurface::CreateWindow(const FWindowDesc& state, bool bPrimary)
{
    GLFWwindow* shareContext = nullptr;

    if (m_PrimaryWindow)
    {
        // We already have a primary window; share its context
        auto primaryGLFW = std::dynamic_pointer_cast<GLFWWindow>(m_PrimaryWindow);
        if (primaryGLFW)
        {
            shareContext = static_cast<GLFWwindow*>(primaryGLFW->GetNativeHandle());
        }
    }

    // Construct the GLFWWindow with the chosen shareContext
    auto window = MakeShared<GLFWWindow>(state, shareContext);

    if (!window->Initialize())
    {
        std::cerr << "[GLFWSurface]: Failed to create GLFWWindow\n";
        return nullptr;
    }

    // Close handler – delegates to surface
    window->SetCloseHandler(
        [this, weakWindow = TWeakPtr(window)]
        (GLFWWindow&)
        {
            auto locked = weakWindow.lock();
            if (!locked)
                return;

            DestroyWindow(locked);
        });

    // Focus callback – updates m_FocusedWindow
    window->SetFocusCallback(
        [this, weakWindow = TWeakPtr<IPlatformWindow>(std::static_pointer_cast<IPlatformWindow>(window))]
        (IPlatformWindow&, bool focused)
        {
            auto locked = weakWindow.lock();

            if (!locked)
                return;

            if (focused)
            {
                m_FocusedWindow = locked;
            }
            else
            {
                auto current = m_FocusedWindow.lock();
                if (current == locked)
                    m_FocusedWindow.reset();
            }
        });

    m_Windows.push_back(window);

    // Set primary window if this is the primary one
    if (!m_PrimaryWindow && bPrimary)
    {
        m_PrimaryWindow = std::static_pointer_cast<IPlatformWindow>(window);
        MakeContextCurrent(window);
    }

    return std::static_pointer_cast<IPlatformWindow>(window);
}

void GLFWSurface::DestroyWindow(const TSharedPtr<IPlatformWindow>& window)
{
    if (!window)
        return;

    auto glfwWindow = std::dynamic_pointer_cast<GLFWWindow>(window);
    if (!glfwWindow)
        return;

    auto focused = m_FocusedWindow.lock();
    if (focused == window)
    {
        m_FocusedWindow.reset();
    }

    // Do not allow destroying the primary mid-session
    if (m_PrimaryWindow == window)
    {
        return;
    }

    // Native destroy via the window's own logic
    glfwWindow->Shutdown();

    // Remove from our list if it’s a GLFWWindow we own
    for (auto it = m_Windows.begin(); it != m_Windows.end(); ++it)
    {
        if (it->get() == glfwWindow.get())
        {
            m_Windows.erase(it);
            break;
        }
    }
}


TSharedPtr<IPlatformWindow> GLFWSurface::GetFocusedWindow() const
{
    return m_FocusedWindow.lock();
}

TSharedPtr<IPlatformWindow> GLFWSurface::GetPrimaryWindow() const
{
    return m_PrimaryWindow;
}

TSharedPtr<IPlatformWindow> GLFWSurface::GetEffectiveInputWindow() const
{
    auto focused = m_FocusedWindow.lock();
    if (focused)
        return focused;

    return m_PrimaryWindow;
}

std::vector<TSharedPtr<IPlatformWindow>> GLFWSurface::GetAllWindows() const
{
    std::vector<TSharedPtr<IPlatformWindow>> result;
    result.reserve(m_Windows.size());

    for (const auto& win : m_Windows)
    {
        if (win) result.push_back(std::static_pointer_cast<IPlatformWindow>(win));
    }

    return result;
}

void GLFWSurface::MakeContextCurrent(const TSharedPtr<IPlatformWindow> &window)
{
    if (!window)
    {
        glfwMakeContextCurrent(nullptr);
        return;
    }

    auto glfwWindow = std::dynamic_pointer_cast<GLFWWindow>(window);
    if (!glfwWindow)
        return;

    void* native = glfwWindow->GetNativeHandle(); // or similar getter
    glfwMakeContextCurrent(static_cast<GLFWwindow*>(native));
}

void GLFWSurface::Present(const TSharedPtr<IPlatformWindow>& window)
{
    SwapBuffers(window);
}

void GLFWSurface::SwapBuffers(const TSharedPtr<IPlatformWindow>& window)
{
    if (!window)
        return;

    auto glfwWindow = std::dynamic_pointer_cast<GLFWWindow>(window);
    if (!glfwWindow)
        return;

    void* native = glfwWindow->GetNativeHandle();
    glfwSwapBuffers(static_cast<GLFWwindow*>(native));
}

void GLFWSurface::PollSurfaceEvents()
{
    glfwPollEvents();
}

float GLFWSurface::GetTimeSeconds()
{
    return static_cast<float>(glfwGetTime());
}

IPlatformSurface::GetProcAddressFunc GLFWSurface::GetProcAddressFunction() const
{
    // Explicitly cast the GLFW function to our expected signature.
    return reinterpret_cast<GetProcAddressFunc>(glfwGetProcAddress);
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
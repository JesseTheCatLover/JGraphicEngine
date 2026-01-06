//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <functional>

enum class EWindowState
{
    Normal,
    Minimized,
    Maximized,
    Fullscreen,
    Hidden,
    Lost
};

struct FSurfaceState
{
    int width = 1280;
    int height = 720;
    bool bvSync = true;
    EWindowState windowState = EWindowState::Maximized;
    std::string title = "JGraphicEngine";
    void* nativeHandle = nullptr;
    void* monitorHandle = nullptr;
};

enum class ECursorMode
{
    Visible,
    Hidden,
    Disabled
};

class IPlatformSurface
{
public:
    using FResizeCallback = std::function<void(int width, int height)>;

public:
    virtual ~IPlatformSurface() = default;

    virtual bool Initialize(const FSurfaceState& state) = 0;
    virtual void Shutdown() = 0;
    [[nodiscard]] virtual bool ShouldClose() const = 0;
    virtual void SetShouldClose(bool bShould) = 0;

    virtual void Present() = 0;
    virtual void SwapBuffers() {} // Optional override, for GL/EGL backends that use implicit swap-chains
    virtual void SetSurfaceSize(int width, int height) = 0;

    virtual void PollSurfaceEvents() = 0;

    virtual void* GetNativeHandle() const = 0;
    [[nodiscard]] virtual int GetWidth() const = 0;
    [[nodiscard]] virtual int GetHeight() const = 0;
    [[nodiscard]] virtual float GetAspectRatio() const = 0;
    virtual void GetFramebufferSize(int& w, int& h) const = 0;
    virtual void GetWindowSize(int& w, int& h) const = 0;
    [[nodiscard]] virtual bool IsVSyncEnabled() const = 0;
    [[nodiscard]] virtual bool IsFullscreen() const = 0;
    [[nodiscard]] virtual FSurfaceState GetState() const = 0;

    virtual void SetTitle(const std::string& title) = 0;
    virtual void SetVSync(bool vSync) = 0;

    virtual void SetCursorMode(ECursorMode mode) {}
    virtual void SetCursorVisible() {}
    virtual void SetCursorHidden() {}
    virtual void SetCursorDisabled() {}

    virtual float GetTimeSeconds() = 0;

    virtual void* GetPlatformSpecificHandle() const { return nullptr; } // optional override

    virtual void SetFramebufferResizeCallback(FResizeCallback callback) {}
    virtual void SetWindowResizeCallback(FResizeCallback callback) {}

    using GetProcAddressFunc = void* (*)(const char*);
    // Optional: for OpenGL-style loaders
    virtual GetProcAddressFunc GetProcAddressFunction() const { return nullptr; }
};
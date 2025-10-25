//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <cmath>

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
    uint32_t width = 1280;
    uint32_t height = 720;
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
    virtual ~IPlatformSurface() = default;

    virtual bool Initialize(const FSurfaceState& state) = 0;
    virtual void Shutdown() = 0;

    virtual void Present() = 0;
    virtual void SwapBuffers() {} // Optional override, for GL/EGL backends that use implicit swap-chains
    virtual void Resize(uint32_t width, uint32_t height) = 0;

    virtual void PollSurfaceEvents() = 0;

    virtual void* GetNativeHandle() const = 0;
    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;
    virtual bool IsVSyncEnabled() const = 0;
    virtual bool IsFullscreen() const = 0;
    virtual FSurfaceState GetState() const = 0;

    virtual void SetTitle(const std::string& title) = 0;
    virtual void SetVSync(bool vSync) = 0;

    virtual void SetCursorMode(ECursorMode mode) {}
    virtual void SetCursorVisible() {}
    virtual void SetCursorHidden() {}
    virtual void SetCursorDisabled() {}

    virtual void* GetPlatformSpecificHandle() const { return nullptr; } // optional override

    using GetProcAddressFunc = void* (*)(const char*);
    // Optional: for OpenGL-style loaders
    virtual GetProcAddressFunc GetProcAddressFunction() const { return nullptr; }
};
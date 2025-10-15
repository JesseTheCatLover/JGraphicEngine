// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "../IPlatformSurface.h"

struct GLFWwindow;

class GLFWSurface : public IPlatformSurface
{
private:
    GLFWwindow* m_Window = nullptr;
    FSurfaceState m_State{};
    ECursorMode m_CursorMode = ECursorMode::Visible;

public:
    GLFWSurface() = default;
    ~GLFWSurface() override { GLFWSurface::Shutdown(); }

    bool Initialize(const FSurfaceState &state) override;

    void Shutdown() override;

    void Present() override;

    void SwapBuffers() override;

    void Resize(uint32_t width, uint32_t height) override;

    void* GetNativeHandle() const override;

    bool IsFullscreen() const override;

    uint32_t GetWidth() const override;

    uint32_t GetHeight() const override;

    void SetCursorMode(ECursorMode mode) override;

    void SetCursorVisible() override;

    void SetCursorHidden() override;

    void SetCursorDisabled() override;

    bool IsVSyncEnabled() const override;

    FSurfaceState GetState() const override;

    void SetTitle(const std::string &title) override;

    void SetVSync(bool vSync) override;

private:
    void UpdateCursor();
};

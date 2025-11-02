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
    ~GLFWSurface() override;

    bool Initialize(const FSurfaceState &state) override;

    void Shutdown() override;

    GetProcAddressFunc GetProcAddressFunction() const override;

    void Present() override;

    void SwapBuffers() override;

    void Resize(int width, int height) override;

    void PollSurfaceEvents() override;

    void* GetNativeHandle() const override;

    bool IsFullscreen() const override;

    int GetWidth() const override;

    int GetHeight() const override;

    void SetCursorMode(ECursorMode mode) override;

    void GetFramebufferSize(int &w, int &h) const override;

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

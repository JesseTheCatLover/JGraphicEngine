// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Rendering/IPlatformSurface.h"

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

    void SetSurfaceSize(int width, int height) override;

    void PollSurfaceEvents() override;

    [[nodiscard]] bool ShouldClose() const override;

    void SetShouldClose(bool bShould) override;

    void GetWindowSize(int &w, int &h) const override;

    void* GetNativeHandle() const override;

    [[nodiscard]] bool IsFullscreen() const override;

    [[nodiscard]] int GetWidth() const override;

    [[nodiscard]] int GetHeight() const override;

    [[nodiscard]] float GetAspectRatio() const override;

    void SetCursorMode(ECursorMode mode) override;

    void GetFramebufferSize(int &w, int &h) const override;

    void SetCursorVisible() override;

    void SetCursorHidden() override;

    void SetCursorDisabled() override;

    [[nodiscard]] bool IsVSyncEnabled() const override;

    FSurfaceState GetState() const override;

    void SetTitle(const std::string &title) override;

    void SetVSync(bool vSync) override;

private:
    void UpdateCursor();
};

// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Rendering/IPlatformSurface.h"

#include "Core/Memory/SmartPointers.h"
#include <vector>

class GLFWWindow;
struct GLFWwindow;

class GLFWSurface : public IPlatformSurface
{
private:
    TSharedPtr<IPlatformWindow> m_PrimaryWindow;
    TWeakPtr<IPlatformWindow> m_FocusedWindow;

    // Track created windows
    std::vector<TSharedPtr<GLFWWindow>> m_Windows;

    bool m_Initialized = false;
    bool m_Shutdown = false;

public:
    GLFWSurface() = default;
    ~GLFWSurface() override;

    bool Initialize() override;
    void Shutdown() override;

    TSharedPtr<IPlatformWindow> CreateWindow(const FWindowDesc& statem, bool bPrimary = false) override;
    void DestroyWindow(const TSharedPtr<IPlatformWindow>& window) override;

    [[nodiscard]] TSharedPtr<IPlatformWindow> GetFocusedWindow() const override;

    [[nodiscard]] TSharedPtr<IPlatformWindow> GetPrimaryWindow() const override;

    [[nodiscard]] TSharedPtr<IPlatformWindow> GetEffectiveInputWindow() const override;

    [[nodiscard]] std::vector<TSharedPtr<IPlatformWindow>> GetAllWindows() const override;

    void MakeContextCurrent(const TSharedPtr<IPlatformWindow> &window) override;

    void Present(const TSharedPtr<IPlatformWindow>& window) override;

    void SwapBuffers(const TSharedPtr<IPlatformWindow>& window) override;

    void PollSurfaceEvents() override;

    float GetTimeSeconds() override;

    GetProcAddressFunc GetProcAddressFunction() const override;

    void* GetPlatformSpecificHandle() const override { return nullptr; }

    // Native file dialogues:

    std::string OpenFileDialog(const char *filterList, const char *defaultPath) override;
    std::vector<std::string> OpenFileDialogMultiple(const char *filterList, const char *defaultPath) override;
    std::string OpenFolderDialog(const char *defaultPath) override;
    std::string SaveFileDialog(const char *filterList, const char *defaultPath, const char* defaultName = nullptr) override;

private:
    void UpdateCursor();
};

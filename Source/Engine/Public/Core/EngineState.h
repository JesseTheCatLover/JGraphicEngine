#pragma once
#include <memory>

#include "Contexts/EViewMode.h"

#include "GLFW/glfw3.h"

class JCamera;
struct FInputContext;
struct FViewportContext;
struct FFrameContext;
struct FWindowContext;

class EngineState
{
private:
    EngineState();
    ~EngineState();

    EngineState(const EngineState&) = delete;
    EngineState& operator=(const EngineState&) = delete;

    friend class JEngine;

    bool m_bRunning = true;

    // TODO: Temporarily make context structs until each part has dedicated managers and subsystems
    std::unique_ptr<FFrameContext> m_FrameContext;
    std::unique_ptr<FWindowContext> m_WindowContext;
    std::unique_ptr<FViewportContext> m_ViewportContext;
    std::unique_ptr<FInputContext> m_InputContext;

public:
    float GetDeltaTime() const;
    void SetDeltaTime(float dt);

    int GetWindowWidth() const;
    void SetWindowWidth(int w);
    int GetWindowHeight() const;
    void SetWindowHeight(int h);

    bool GetWireframeMode();
    void SetWireframeMode(bool bWireMode);
    EViewMode GetViewMode() const;
    void SetViewMode(EViewMode mode);
    JCamera* GetCamera() const;
    void SetCamera(JCamera* camera);
    FViewportContext* GetCameraSettings() const;

private:
    void SetRunning(bool bIsRunning) { m_bRunning = bIsRunning; }
    bool GetIsRunning() const { return m_bRunning; }

    float GetLastFrameTime() const;
    void SetLastFrameTime(float lft);

    GLFWwindow* GetGLFWWindow() const;
    void SetGLFWWindow(GLFWwindow* window);

    bool GetIsFirstMouse();
    void SetIsFirstMouse(bool bIsFirst);

    float GetLastMouseX();
    void SetLastMouseX(float x);
    float GetLastMouseY();
    void SetLastMouseY(float y);

};

#pragma once
#include <memory>

#include "Contexts/EViewMode.h"

#include "GLFW/glfw3.h"

#include "Memory/SmartPointers.h"

class ICameraViewSource;
struct FInputContext;
struct FViewportContext;
struct FFrameContext;
struct FSurfaceContext;

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
    TUniquePtr<FFrameContext> m_FrameContext;
    TUniquePtr<FSurfaceContext> m_SurfaceContext;
    TUniquePtr<FViewportContext> m_ViewportContext;
    TUniquePtr<FInputContext> m_InputContext;

public:
    [[nodiscard]] const float& GetDeltaTime() const;
    void SetDeltaTime(float dt);

    int GetFramebufferWidth() const;
    void SetFramebufferWidth(int w);
    int GetFramebufferHeight() const;
    void SetFramebufferHeight(int h);
    float GetAspectRatio() const;
    bool GetIsSurfaceFullscreen();

    bool GetWireframeMode();
    void SetWireframeMode(bool bWireMode);
    EViewMode GetViewMode() const;
    void SetViewMode(EViewMode mode);
    ICameraViewSource* GetCamera() const;
    void SetCamera(ICameraViewSource* camera);
    FViewportContext* GetCameraSettings() const;

private:
    void SetRunning(bool bIsRunning) { m_bRunning = bIsRunning; }
    bool GetIsRunning() const { return m_bRunning; }

    float GetLastFrameTime() const;
    void SetLastFrameTime(float lft);

    bool GetIsFirstMouse();
    void SetIsFirstMouse(bool bIsFirst);

    float GetLastMouseX();
    void SetLastMouseX(float x);
    float GetLastMouseY();
    void SetLastMouseY(float y);

};

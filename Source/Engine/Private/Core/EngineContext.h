#pragma once
#include "Core/Memory/SmartPointers.h"

class ICameraViewSource;
struct FInputContext;
struct FViewportContext;
struct FFrameContext;
struct FSurfaceContext;

class EngineContext
{
    friend class JEngine;

private:
    EngineContext();

    bool m_bRunning = true;

    TUniquePtr<FFrameContext> m_FrameContext;
    TUniquePtr<FSurfaceContext> m_SurfaceContext;
    TUniquePtr<FViewportContext> m_ViewportContext;
    TUniquePtr<FInputContext> m_InputContext;

public:
    ~EngineContext();

    EngineContext(const EngineContext&) = delete;
    EngineContext& operator=(const EngineContext&) = delete;

    [[nodiscard]] const float& GetDeltaTime() const;
    void SetDeltaTime(float dt);

    [[nodiscard]] int GetFramebufferWidth() const;
    void SetFramebufferWidth(int w);
    [[nodiscard]] int GetFramebufferHeight() const;
    void SetFramebufferHeight(int h);
    [[nodiscard]] float GetAspectRatio() const;
    bool GetIsSurfaceFullscreen();

    bool GetWireframeMode();
    void SetWireframeMode(bool bWireMode);
    [[nodiscard]] ICameraViewSource* GetCamera() const;
    void SetCamera(ICameraViewSource* camera);
    [[nodiscard]] FViewportContext* GetCameraSettings() const;

private:
    void SetRunning(bool bIsRunning) { m_bRunning = bIsRunning; }
    [[nodiscard]] bool GetIsRunning() const { return m_bRunning; }

    [[nodiscard]] float GetLastFrameTime() const;
    void SetLastFrameTime(float lft);

    bool GetIsFirstMouse();
    void SetIsFirstMouse(bool bIsFirst);

    float GetLastMouseX();
    void SetLastMouseX(float x);
    float GetLastMouseY();
    void SetLastMouseY(float y);

};

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

    // TODO: TEMP
    ICameraViewSource* m_CurrentCamera   = nullptr;
    float              m_CurrentAspect   = 16.0f / 9.0f;

public:
    ~EngineContext();

    EngineContext(const EngineContext&) = delete;
    EngineContext& operator=(const EngineContext&) = delete;

    [[nodiscard]] const float& GetDeltaTime() const;
    void SetDeltaTime(float dt);

    [[nodiscard]] int GetFramebufferWidth() const;
    [[nodiscard]] int GetFramebufferHeight() const;
    void SetFramebufferSize(int w, int h);
    bool GetIsSurfaceFullscreen();

    bool GetShouldRenderToPlatformSurface() const;
    void SetShouldRenderToPlatformSurface(bool bShould);

    bool GetWireframeMode();
    void SetWireframeMode(bool bWireMode);

    // TODO: TEMP
    void SetCamera(ICameraViewSource* camera, float aspect);
    [[nodiscard]] ICameraViewSource* GetCamera() const;
    [[nodiscard]] float GetAspectRatio() const;

    [[nodiscard]] int GetSceneViewportWidth() const;
    [[nodiscard]] int GetSceneViewportHeight() const;
    void SetSceneViewportSize(int w, int h);

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

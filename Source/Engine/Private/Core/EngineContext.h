#pragma once
#include <vector>
#include "Core/Memory/SmartPointers.h"
#include "Rendering/FRenderView.h"

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
    std::vector<FRenderView> m_ViewSources;

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
    [[nodiscard]] int GetFramebufferHeight() const;
    void SetFramebufferSize(int w, int h);
    bool GetIsSurfaceFullscreen();

    bool GetShouldRenderToPlatformSurface() const;
    void SetShouldRenderToPlatformSurface(bool bShould);

    bool GetWireframeMode();
    void SetWireframeMode(bool bWireMode);

    void AddViewSource(const FRenderView& view) { m_ViewSources.push_back(view); }

private:
    void SetRunning(bool bIsRunning) { m_bRunning = bIsRunning; }
    [[nodiscard]] bool GetIsRunning() const { return m_bRunning; }

    [[nodiscard]] const std::vector<FRenderView>& GetViewSources() const { return m_ViewSources; }
    void ClearViewSources() { m_ViewSources.clear(); }

    [[nodiscard]] float GetLastFrameTime() const;
    void SetLastFrameTime(float lft);

    bool GetIsFirstMouse();
    void SetIsFirstMouse(bool bIsFirst);

    float GetLastMouseX();
    void SetLastMouseX(float x);
    float GetLastMouseY();
    void SetLastMouseY(float y);

};

#pragma once
#include <vector>
#include <cstdint>
#include <unordered_set>

#include "Core/Memory/SmartPointers.h"
#include "Rendering/FRenderView.h"

struct FEditorSelectionRenderState
{
    std::unordered_set<uint64_t> selectedActors; // actorIDs
    uint8_t selectionStencil = 1;          // 1..255

    void SetSelectedActors(const std::vector<uint64_t>& ids)
    {
        selectedActors.clear();
        selectedActors.reserve(ids.size());
        for (uint64_t id : ids)
            if (id != 0) selectedActors.insert(id);
    }

    [[nodiscard]] bool IsSelected(uint64_t id) const
    {
        return selectedActors.find(id) != selectedActors.end();
    }

    [[nodiscard]] bool HasSelection() const
    {
        return !selectedActors.empty();
    }
};

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

    FEditorSelectionRenderState m_EditorSelection;

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

    FEditorSelectionRenderState& GetEditorSelectionState() { return m_EditorSelection; }
    const FEditorSelectionRenderState& GetEditorSelectionState() const { return m_EditorSelection; }

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

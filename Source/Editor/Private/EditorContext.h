//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include "Scene/FEditorActorSnapshot.h"
#include "Viewport/FEditorFrameSnapshot.h"

enum class EGizmoMode
{
    Select,
    Translate,
    Rotate,
    Scale
};

class EngineContext;
class EditorCore;

class EditorContext
{
    friend class EditorCore;
private:
    std::vector<ActorID> m_SelectedActors;
    EGizmoMode m_GizmoMode = EGizmoMode::Select;

    bool m_ShowAssetBrowser = true;
    bool m_ShowInspector = true;

    FEditorFrameSnapshot m_FrameSnapshot{};

public:
    EditorContext() = default;

    [[nodiscard]] const std::vector<ActorID>& GetSelection() const
    {
        return m_SelectedActors;
    }

    [[nodiscard]] bool GetIsSelected(ActorID id) const
    {
        for (auto sel : m_SelectedActors)
            if (sel == id) return true;
        return false;
    }

    [[nodiscard]] EGizmoMode GetGizmoMode() const { return m_GizmoMode; }

    [[nodiscard]] bool GetIsAssetBrowserVisible() const { return m_ShowAssetBrowser; }

    [[nodiscard]] const FEditorFrameSnapshot& GetFrameSnapshot() const
    {
        return m_FrameSnapshot;
    }

    // add: active viewport ID, play state, etc.

private:
    void SetSelection(const std::vector<ActorID>& ids)
    {
        m_SelectedActors = ids;
    }

    void SetGizmoMode(EGizmoMode mode) { m_GizmoMode = mode; }

    void SetAssetBrowserVisible(bool visible) { m_ShowAssetBrowser = visible; }

    void SetFrameSnapshot(const FEditorFrameSnapshot& snapshot)
    {
        m_FrameSnapshot = snapshot;
    }
};

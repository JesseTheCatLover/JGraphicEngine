// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorPanelTracker.h"

#include "Core/EditorHost.h"
#include "UI/Panels/InspectorPanel.h"
#include "UI/Panels/ViewportPanel.h"
#include "UI/Panels/SceneHierarchyPanel.h"
#include "UI/Panels/AssetBrowserPanel.h"

void EditorPanelTracker::Initialize(EditorHost& host)
{
    m_PanelsOwned.clear();
    m_DrawPanels.clear();

    // Create initial required panels
    m_PanelsOwned.emplace_back(MakeUnique<ViewportPanel>(0));

    for (auto& p : m_PanelsOwned)
        if (p) p->OnCreate(host);
}

void EditorPanelTracker::Shutdown(EditorHost& host)
{
    for (auto& p : m_PanelsOwned)
        if (p) p->OnDestroy(host);

    m_PanelsOwned.clear();
    m_DrawPanels.clear();
}

IEditorPanel* EditorPanelTracker::FindByKey(const char* key) const
{
    for (auto& p : m_PanelsOwned)
    {
        if (!p) continue;
        if (std::strcmp(p->GetPanelKey(), key) == 0)
            return p.get();
    }
    return nullptr;
}

void EditorPanelTracker::ApplyLayout(EditorHost& host, EditorLayoutModel& layout)
{
    bool anyChanged = false;

    // --- Viewports ---
    if (layout.ConsumeViewportCountChanged())
    {
        anyChanged = true;
        const int desired = layout.GetViewportCount();

        // Ensure Viewport:0 exists
        if (!FindByKey("Viewport:0"))
        {
            auto vp0 = MakeUnique<ViewportPanel>(0);
            vp0->OnCreate(host);
            m_PanelsOwned.emplace_back(std::move(vp0));
        }

        // Create Viewport1..Viewport{desired-1}
        for (int i = 1; i < desired; ++i)
        {
            auto desc = layout.GetViewportDesc(i);
            if (!FindByKey(desc.key.data()))
            {
                auto vp = MakeUnique<ViewportPanel>(i);
                vp->OnCreate(host);
                m_PanelsOwned.emplace_back(std::move(vp));
            }
        }

        // Destroy Viewport{desired}..Viewport3
        for (int i = 3; i >= desired; --i)
        {
            auto desc = layout.GetViewportDesc(i);
            const char* key = desc.key.data();

            for (size_t pi = 0; pi < m_PanelsOwned.size(); ++pi)
            {
                auto& p = m_PanelsOwned[pi];
                if (!p) continue;
                if (std::strcmp(p->GetPanelKey(), key) != 0)
                    continue;

                p->OnDestroy(host);
                m_PanelsOwned.erase(m_PanelsOwned.begin() + (ptrdiff_t)pi);
                break;
            }
        }
    }

    // --- Single panels ---
    for (int i = 0; i < (int)EEditorPanelType::Count; ++i)
    {
        const auto id = (EEditorPanelType)i;
        if (!layout.ConsumePanelVisibilityChanged(id))
            continue;

        anyChanged = true;
        const bool wantVisible = layout.IsPanelVisible(id);
        const auto desc = layout.GetSinglePanelDesc(id);

        if (wantVisible)
        {
            if (!FindByKey(desc.key.data()))
            {
                // Construct by type
                switch (id)
                {
                    case EEditorPanelType::SceneHierarchy:
                    {
                        auto p = MakeUnique<SceneHierarchyPanel>();
                        p->OnCreate(host);
                        m_PanelsOwned.emplace_back(std::move(p));
                        break;
                    }
                    case EEditorPanelType::Inspector:
                    {
                        auto p = MakeUnique<InspectorPanel>();
                        p->OnCreate(host);
                        m_PanelsOwned.emplace_back(std::move(p));
                        break;
                    }
                    case EEditorPanelType::AssetBrowser:
                    {
                        auto p = MakeUnique<AssetBrowserPanel>();
                        p->OnCreate(host);
                        m_PanelsOwned.emplace_back(std::move(p));
                        break;
                    }
                    // case EEditorPanelType::Console: { ... } break;
                    default: break;
                }
            }
        }
        else
        {
            // Destroy it when toggled off (simple for now)
            const char* key = desc.key.data();
            for (size_t pi = 0; pi < m_PanelsOwned.size(); ++pi)
            {
                auto& p = m_PanelsOwned[pi];
                if (!p) continue;
                if (std::strcmp(p->GetPanelKey(), key) != 0)
                    continue;

                p->OnDestroy(host);
                m_PanelsOwned.erase(m_PanelsOwned.begin() + (ptrdiff_t)pi);
                break;
            }
        }
    }

    if (anyChanged)
        RebuildDrawList(layout);
    else if (m_DrawPanels.empty())
        RebuildDrawList(layout);
}

void EditorPanelTracker::RebuildDrawList(EditorLayoutModel& layout)
{
    m_DrawPanels.clear();
    m_DrawPanels.reserve(m_PanelsOwned.size());

    // Visible tool panels governed by layout
    // Viewports governed by existence (we create only needed count)
    for (auto& p : m_PanelsOwned)
    {
        if (!p) continue;

        const char* key = p->GetPanelKey();

        // Filter tool panels by layout visibility (by key)
        if (std::strcmp(key, "SceneHierarchy") == 0 &&
            !layout.IsPanelVisible(EEditorPanelType::SceneHierarchy))
            continue;

        if (std::strcmp(key, "Inspector") == 0 &&
            !layout.IsPanelVisible(EEditorPanelType::Inspector))
            continue;

        if (std::strcmp(key, "AssetBrowser") == 0 &&
            !layout.IsPanelVisible(EEditorPanelType::AssetBrowser))
            continue;

        if (std::strcmp(key, "Console") == 0 &&
            !layout.IsPanelVisible(EEditorPanelType::Console))
            continue;

        m_DrawPanels.push_back(p.get());
    }
}
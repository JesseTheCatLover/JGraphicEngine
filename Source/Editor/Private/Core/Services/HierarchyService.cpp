//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "HierarchyService.h"

#include <functional>

#include "SceneQueryService.h"
#include "SelectionService.h"
#include "Core/EditorHost.h"

HierarchyService::HierarchyService(EditorHost &host)
: m_Host(host)
{}

void HierarchyService::Tick(float x)
{
    IEditorService::Tick(x);
    {
        if (!m_Dirty) return;
        m_Dirty = false;

        auto& queries = m_Host.GetService<SceneQueryService>();
        auto& sel = m_Host.GetService<SelectionService>();

        m_Snapshot = queries.BuildHierarchySnapshot();

        // mark selection
        const auto& selected = sel.GetSelection();
        for (auto& n : m_Snapshot)
            n.isSelected = (std::find(selected.begin(), selected.end(), n.id) != selected.end());

        RebuildVisibleOrder();
    }
}

void HierarchyService::RebuildVisibleOrder()
{
    m_VisibleOrder.clear();
    m_VisibleOrder.reserve(m_Snapshot.size());

    std::function<void(uint64_t)> rec = [&](uint64_t parent)
    {
        for (const auto& n : m_Snapshot)
        {
            if (n.parentID == parent)
            {
                m_VisibleOrder.push_back(n.id);
                if (n.hasChildren) rec(n.id);
            }
        }
    };
    rec(0);
}

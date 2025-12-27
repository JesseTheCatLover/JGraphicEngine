//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <cstdint>

#include "Core/IEditorService.h"
#include "Scene/FHierarchySnapshot.h"

class EditorHost;

class HierarchyService : public IEditorService
{
private:
    EditorHost& m_Host;
    bool m_Dirty = true;
    std::vector<FHierarchySnapshot> m_Snapshot;
    std::vector<uint64_t> m_VisibleOrder;

    void RebuildVisibleOrder();

public:
    explicit HierarchyService(EditorHost& host);

    void Tick(float) override;

    void MarkDirty() { m_Dirty = true; }

    [[nodiscard]] const std::vector<FHierarchySnapshot>& GetSnapshot() const { return m_Snapshot; }
    [[nodiscard]] const std::vector<uint64_t>& GetVisibleOrder() const { return m_VisibleOrder; }
};

// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <vector>
#include <span>
#include <cstring>

#include "Core/Memory/SmartPointers.h"
#include "UI/IEditorPanels.h"
#include "Layout/EditorLayoutModel.h"

class EditorHost;

class EditorPanelTracker
{
private:
    std::vector<TUniquePtr<IEditorPanel>> m_PanelsOwned;
    std::vector<IEditorPanel*> m_DrawPanels; // cached visible list

public:
    void Initialize(EditorHost& host);
    void Shutdown(EditorHost& host);

    // Apply diffs from layout model (consumes dirty flags)
    void ApplyLayout(EditorHost& host, EditorLayoutModel& layout);

    // Renderer consumes this list; valid until next ApplyLayout()
    [[nodiscard]] std::span<IEditorPanel* const> GetDrawPanels() const { return m_DrawPanels; }

private:
    IEditorPanel* FindByKey(const char* key) const;
    void RebuildDrawList(EditorLayoutModel& layout);
};
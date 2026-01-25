//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <unordered_set>

#include "UI/IEditorPanels.h"
#include "Controllers/Inputs/FInspectorPanelInput.h"

struct FInspectorCategorySnapshot;
struct FInspectorOutput;
struct FInspectorSnapshot;

class InspectorPanel : public IEditorPanel
{
    std::string m_PanelKey = "Inspector";

    // UI-only persistent state
    std::unordered_set<size_t> m_OpenCategories;

    static size_t HashCategory(const std::string& s);

    void DrawSnapshot(const FInspectorSnapshot& snap);
    void DrawCategory(size_t objectIndex, const char* name, const FInspectorCategorySnapshot& cat);
    void DrawRow(const struct FInspectorRow& row);

public:
    [[nodiscard]] const char* GetName() const override { return "Inspector###Inspector"; }
    [[nodiscard]] const char* GetPanelKey() const override { return m_PanelKey.c_str(); }

    [[nodiscard]] EPanelDockGroup GetDockGroup() const override { return EPanelDockGroup::Single; }

    void OnDestroy(EditorHost& host) override;
    void Draw(EditorHost& host) override;
};
//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "UI/IEditorPanels.h"
#include "Controllers/Inputs/FInspectorPanelInput.h"
#include "Controllers/Documents/FInspectorDocument.h"

class EditorHost;

class InspectorPanel final : public IEditorPanel
{
public:
    [[nodiscard]] const char* GetName() const override { return "Inspector###Inspector"; }
    [[nodiscard]] const char* GetPanelKey() const override { return "Inspector"; }
    [[nodiscard]] EPanelDockGroup GetDockGroup() const override { return EPanelDockGroup::Single; }

    void OnDestroy(EditorHost& host) override;
    void Draw(EditorHost& host) override;

private:
    // ---------------- UI persistent state ----------------

    std::unordered_set<uint64_t> m_CollapsedCategoryKeys;     // collapsed category headers (default is open)
    std::unordered_map<uint64_t, std::string> m_StringEdits;  // per-row string edit buffer
    std::string m_ActorNameEdit;
    uint64_t    m_ActorNameTargetID = 0; // tracks which actor name is cached
    bool m_bComponentSectionOpen = true;
    float m_ComponentSectionHeight = 150.0f;
    float m_PropertyLabelColumnWidth = 90.0f;
    uint64_t m_SelectedTargetID = 0;
    char     m_SearchBuf[128] = {};

    std::unordered_map<uint64_t, bool> m_RowWasActive;

private:
    // ---------------- helpers ----------------
    static bool MatchesRowSearch(const FInspectorRow& row, const char* search);

    static const FInspectorTarget* FindTargetByID(const FInspectorDocument& doc, uint64_t targetID);
    static const FInspectorTarget* FindActorTarget(const FInspectorDocument& doc);

    // scene subtree traversal (only SceneComponent group)
    static void BuildSceneComponentChildrenMap(
        const FInspectorDocument& doc,
        std::unordered_map<uint64_t, std::vector<const FInspectorTarget*>>& outChildren);

private:
    // ---------------- drawing ----------------
    void DrawActorHeader(const FInspectorDocument& doc, FInspectorPanelInput& input);

    void DrawComponentSection(const FInspectorDocument& doc);

    // Properties UI: draw top header rows (Essentials) + categories
    void DrawPropertiesForSelection(const FInspectorDocument& doc, FInspectorPanelInput& input);

    void DrawHeaderRows(const std::vector<const FInspectorRow*>& rows, FInspectorPanelInput& input);

    void DrawFilteredMergedCategories(
        const std::vector<const FInspectorTarget*>& targetsInOrder,
        FInspectorPanelInput& input);

    void DrawCategorySection(uint64_t categoryID, const std::string& categoryName,
                            const std::vector<const FInspectorRow*>& rows,
                            FInspectorPanelInput& input);

    void DrawRow(const FInspectorRow& row, FInspectorPanelInput& input);
};
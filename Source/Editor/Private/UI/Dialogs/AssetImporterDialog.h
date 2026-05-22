//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <vector>

#include "UI/IEditorDialog.h"

class AssetImporterDialog final : public IEditorDialog
{
private:
    bool m_bIsOpen = false;
    bool m_bRequestFocus = false;

    float m_InitialWidth = 800.f;
    float m_InitialHeight = 500.f;

    float m_MinWidth = 600.0f;
    float m_MinHeight = 400.0f;
    float m_MaxWidth = 1600.0f;
    float m_MaxHeight = 1200.0f;

    bool m_bInitializedSplitter = false;
    float m_LeftPaneWidth = -1.0f;
    const float m_MinLeftPaneWidth = 220.0f;
    const float m_MinRightPaneWidth = 280.0f;

    struct FPendingItem
    {
        std::string sourceFilePath;
        std::string destinationVirtualFolder;
        bool bOverwrite = false;
        // later: per-type options
    };

    // Pending items
    std::vector<FPendingItem> m_Items;

    // List selection (indices into m_Items)
    std::vector<int> m_SelectedIndices;

    // Left pane behavior
    void DrawLeftPane(EditorHost& host, EditorRuntime& runtime);
    void DrawLeftTopBar(EditorHost& host, EditorRuntime& runtime);
    void DrawLeftItemList(EditorHost& host, EditorRuntime& runtime);
    void DrawLeftBottomBar(EditorHost& host, EditorRuntime& runtime);

    // Right pane behavior
    void DrawRightPane(EditorHost& host, EditorRuntime& runtime);

    // Actions
    void OnBrowseSourceFiles(EditorHost& host, EditorRuntime& runtime);
    void OnChooseDestinationForSelected(EditorHost& host, EditorRuntime& runtime);
    void OnDeleteSelected();
    void OnCancel();
    void OnDone(EditorHost& host, EditorRuntime& runtime);

    // Helpers
    void BuildImportRequestsAndSubmit(EditorHost& host, EditorRuntime& runtime);
    void EnsureSelectionIsValid();
    void UpdateFolderPickerResult(EditorHost& host, EditorRuntime& runtime);

public:
    AssetImporterDialog() = default;

    [[nodiscard]] const char* GetName() const override { return "AssetImporterDialog"; }

    // Called when dialog is created once
    void OnCreate(EditorHost& host, EditorRuntime& runtime) override;

    void OnDestroy(EditorHost& host, EditorRuntime& runtime) override;

    // Called every time user wants to open it
    void OnOpen(EditorHost& host, EditorRuntime& runtime) override;

    void OnRequestFocus(EditorHost &host, EditorRuntime &runtime) override { m_bRequestFocus = true; }

    void Draw(EditorHost& host, EditorRuntime& runtime) override;
    [[nodiscard]] bool IsOpen() const override { return m_bIsOpen; }
};

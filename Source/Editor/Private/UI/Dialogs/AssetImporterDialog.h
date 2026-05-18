//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <vector>

#include "UI/IEditorDialog.h"
#include "Assets/FAssetImportRequest.h"
#include "Assets/FAssetImportResult.h"

class AssetImporterDialog final : public IEditorDialog
{
public:
    AssetImporterDialog() = default;

    const char* GetName() const override { return "AssetImporterDialog"; }

    // Called when dialog is created once
    void OnCreate(EditorHost& host, EditorRuntime& runtime) override {}
    void OnDestroy(EditorHost& host, EditorRuntime& runtime) override {}

    // Called every time user wants to open it
    void OnOpen(EditorHost& host, EditorRuntime& runtime) override;

    void Draw(EditorHost& host, EditorRuntime& runtime) override;
    bool IsOpen() const override { return m_IsOpen; }

private:
    bool m_IsOpen = false;
    bool m_InitialPopupOpened = false;

    struct PendingItem
    {
        std::string sourceFilePath;
        std::string destinationVirtualFolder;
        bool bOverwrite = false;
        // later: per-type options
    };

    std::vector<PendingItem> m_Items;
    int m_SelectedIndex = -1;
};

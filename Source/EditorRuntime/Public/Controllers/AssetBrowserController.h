//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <vector>

#include "PanelRegistry.h"
#include "Documents/FAssetBrowserDocument.h"

struct FAssetBrowserOutput;
struct FAssetBrowserPanelInput;
class EditorHost;
class EditorRuntime;
class EditorFileAPI;

class AssetBrowserController
{
private:
    // --- Core editor context ---
    PanelID m_PanelID = 0;
    EditorHost& m_Host;
    EditorRuntime& m_Runtime;

    EditorFileAPI& m_FileAPI;

    // --- Document / state ---
    FAssetBrowserDocument m_Document;
    bool m_Dirty = true;

    // Internal helpers
    void BuildDirectories();
    void BuildAssets();

    [[nodiscard]] bool IsDirectChildDirectory(const std::string& parentDir, const std::string& assetVirtualPath,
        std::string& outChildDirName, std::string& outChildDirVirtualPath) const;

    [[nodiscard]] bool IsDirectChildAsset(const std::string& parentDir, const std::string& assetVirtualPath) const;

    [[nodiscard]] static std::string ComputeParentPath(const std::string& path);

public:
    AssetBrowserController(PanelID id, EditorHost& host, EditorRuntime& runtime);
    ~AssetBrowserController();

    // --- Panel-facing API ---

    // Called once per frame (or per panel draw) by the panel
    // Ensures the document is up-to-date
    void Refresh();

    // Navigation: panel calls this when user navigates to a different folder
    void SetCurrentPath(const std::string& path);

    // Convenience accessor
    const std::string& GetCurrentPath() const { return m_Document.currentPath; }

    // The panel uses this to render the folder contents
    const FAssetBrowserDocument& GetDocument() const { return m_Document; }

    // For external invalidation (e.g. asset imported / registry changed)
    void Invalidate() { m_Dirty = true; }

    void Update(float deltaTime, const FAssetBrowserPanelInput& input, FAssetBrowserOutput& out);
    void OnPanelDestroyed();

};

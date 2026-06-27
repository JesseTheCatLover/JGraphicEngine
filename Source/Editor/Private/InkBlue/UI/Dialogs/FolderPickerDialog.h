//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <functional>

#include "Core/Delegates/FDelegateHandle.h"
#include "Core/Memory/SmartPointers.h"
#include "EditorCore/Services/AssetBrowser/FAssetBrowserViewProjection.h"
#include "EditorCore/IEditorDialog.h"

struct FAssetBrowserViewSettings;
class AssetBrowserService;
class AssetBrowserViewController;
class EditorHost;
class EditorRuntime;

class FolderPickerDialog final : public IEditorDialog
// Note: After many wasted time on this, i finally
// call this done...
// yes it has many bugs but that's what you get with this immediate-mode structure...; gotta skip for now
{
private:
    // Lifetime
    bool m_bIsOpen = false;
    bool m_bJustOpened = false;  // for one-shot focus

    // Services
    AssetBrowserService* m_Service = nullptr;

    FDelegateHandle m_AssetsMutatedHandle;
    FDelegateHandle m_PendingFoldersModifierHandle;

    TUniquePtr<AssetBrowserViewController> m_Controller;

    struct FPendingFolderState
    {
        std::unordered_set<std::string> pendingPaths;

        std::string renameTargetPath;
        std::string renameBuffer;

        bool bStartRenameFocus = false;
    };

    // Pending virtual folders
    FPendingFolderState m_PendingFolders;
    AssetBrowserNodeID m_NextPendingNodeID = -1;
    std::unordered_set<AssetBrowserNodeID> m_PendingNodeIDs;
    std::unordered_map<std::string, AssetBrowserNodeID> m_PendingNodeIDsByPath;

    // Current selection/path
    std::string m_PathInputBuffer;

    // Callbacks
    std::function<void(const std::string&)> m_OnAccepted;

    // Layout
    std::string m_Title = "Select Destination Folder";

    float m_InitialWidth = 300.0f;
    float m_InitialHeight = 500.0f;

    float m_MinWidth = 400.0f;
    float m_MinHeight = 300.0f;
    float m_MaxWidth = 600.0f;
    float m_MaxHeight = 900.0f;

public:
    FolderPickerDialog();
    ~FolderPickerDialog() override;

    void OnCreate(EditorHost& host, EditorRuntime& runtime) override;
    void OnDestroy(EditorHost& host, EditorRuntime& runtime) override;
    void OnOpen(EditorHost& host, EditorRuntime& runtime) override;
    void OnClose() override;
    void OnRequestFocus(EditorHost& host, EditorRuntime& runtime) override;
    void Draw(EditorHost& host, EditorRuntime& runtime) override;

    [[nodiscard]] const char* GetName() const override { return "FolderPickerDialog"; }
    [[nodiscard]] bool IsOpen() const override { return m_bIsOpen; }

    // Optionally let caller override title
    void SetTitle(const std::string& title) { m_Title = title; }

    // Let caller manipulate the destination path
    void SetDestinationPathOnPicker(const std::string& path);

    // Set callback on folder selection
    void SetOnAccepted(std::function<void(const std::string&)> callback) { m_OnAccepted = std::move(callback); }

private:
    // View lifecycle
    void RefreshView(EditorHost& host, EditorRuntime& runtime);
    void ApplyPendingFolderModifier(FAssetBrowserViewProjection& projection);

    // UI
    void DrawContent(AssetBrowserService& service);
    void DrawTopBar();
    void DrawDirectoryList();
    void DrawBottomBar(AssetBrowserService& service);

    void DrawNode(AssetBrowserNodeID id);
    void DrawPendingRenameWidget();

    // Navigation
    void SetNavigationToPath(const std::string& rawPath);
    bool NavigateToPath(AssetBrowserService& service, const std::string& path, bool bAllowCreatePending);
    void SyncInputBufferWithCurrentNavigation();
    bool ApplyPathInput(AssetBrowserService& service);

    void EnsurePathVisible(AssetBrowserService& service, const std::string& path);

    void ExpandAll();
    void CollapseAll();

    // Pending folder model
    AssetBrowserNodeID GetOrCreatePendingNodeID(const std::string& path);
    void RemovePendingNodeID(const std::string& path);

    bool AddPendingFolderPath(const std::string& rawPath);
    void RemovePendingFolderPathAndDescendants(const std::string& path);

    bool CommitPendingFolders(AssetBrowserService& service);
    bool CommitPendingRename(const std::string& targetPath, const std::string& newName);

    void EndPendingRename();
    void CancelPendingRename();

    std::string OnCreateFolderClicked();

    // Node interactions
    void HandlePendingNodeInteractions(AssetBrowserNodeID id, const FAssetBrowserNode& node);

    bool HandlePendingDeleteButton(const std::string& nodePath, int id);

    // Queries
    [[nodiscard]] bool IsRenaming(const std::string& path) const;
    [[nodiscard]] bool IsPendingNode(const std::string& path) const;
    [[nodiscard]] bool IsPendingNodeID(AssetBrowserNodeID id) const;
    [[nodiscard]] bool IsRealFolderPresent(const std::string& path) const;
    [[nodiscard]] bool PathExistsInPicker(const std::string& path) const;
    [[nodiscard]] bool FolderHasChildren(AssetBrowserNodeID id) const;

    [[nodiscard]] std::string BuildNodeLabel(const FAssetBrowserNode& node) const;

    [[nodiscard]] std::string MakeUniqueChildPath(const std::string& parent, const std::string& baseName) const;

    // Projection access
    [[nodiscard]] const FAssetBrowserViewProjection& GetProjection() const;
    [[nodiscard]] const FAssetBrowserViewSettings& GetSettings() const;
    [[nodiscard]] const std::string& GetCurrentNavigationPath() const;

    AssetBrowserNodeID GetRootNodeID(EditorHost& host) const;

    // Styling
    void PushPendingNodeStyle();
    void PopPendingNodeStyle();

    static bool IsRootPath(const std::string& path);
};

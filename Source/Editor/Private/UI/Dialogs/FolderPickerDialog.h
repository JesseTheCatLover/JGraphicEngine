//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <unordered_set>
#include <functional>

#include "Core/Delegates/FDelegateHandle.h"
#include "Core/Services/AssetBrowser/FAssetBrowserViewState.h"
#include "UI/IEditorDialog.h"

class EditorHost;
class EditorRuntime;

class FolderPickerDialog final : public IEditorDialog
{
private:
    // Lifetime
    bool m_bIsOpen = false;
    bool m_bJustOpened = false;  // for one-shot focus
    bool m_bDirty = true;

    // Services
    EditorHost* m_Host = nullptr;
    FDelegateHandle m_AssetsMutatedHandle;

    // Asset browser projection
    FAssetBrowserViewState m_View;

    struct FPendingFolderState
    {
        std::unordered_set<std::string> pendingPaths;

        AssetBrowserNodeID renameTargetID = 0;
        std::string renameBuffer;

        bool bStartRenameFocus = false;
    };

    // Pending virtual folders
    FPendingFolderState m_Pending;
    static constexpr AssetBrowserNodeID PendingNodeMask = 0x8000000000000000ULL;
    static constexpr AssetBrowserNodeID InvalidNodeID = 0;
    std::unordered_set<std::string> m_ExpandedPendingNodes;

    // Current selection/path
    std::string m_CurrentPath = "/Project"; // default virtual folder path
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
    FolderPickerDialog() = default;

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
    void RefreshViewIfDirty(EditorHost& host, EditorRuntime& runtime);
    void ApplyExpansionRequests();

    void DrawContent(EditorHost& host, EditorRuntime& runtime);
    void DrawTopBar();
    void DrawDirectoryList();
    void DrawBottomBar(EditorHost& host);

    void SetCurrentPath(const std::string& path);

    void DrawNodeByID(AssetBrowserNodeID id);
    [[nodiscard]] std::string BuildNodeLabel( AssetBrowserNodeID id, const FAssetBrowserNode& node) const;
    void DrawPendingRenameWidget(AssetBrowserNodeID id);

    void PushPendingNodeStyle();
    void PopPendingNodeStyle();

    void HandlePendingNodeInteractions( AssetBrowserNodeID id, const FAssetBrowserNode& node);
    bool HandlePendingDeleteButton(const std::string& nodePath);

    void ExpandAll();
    void CollapseAll();

    void SyncPathInputToCurrentPath();
    bool ApplyPathInput(EditorHost& host);

    bool AddPendingFolderPath(const std::string& rawPath);
    void RemovePendingFolderPathAndDescendants(const std::string& normalizedPath);
    bool CommitPendingFolders(EditorHost& host);
    bool CommitPendingRename(AssetBrowserNodeID nodeID, const std::string& newName);

    void EndPendingRename();
    void CancelPendingRename();

    [[nodiscard]] std::string MakeUniqueChildPath(const std::string& parent, const std::string& baseName) const;

    std::string OnCreateFolderClicked();

    static bool IsRootPath(const std::string& path);
    [[nodiscard]] bool IsFolderPresentInUI(const std::string& normalizedPath) const;
    bool FolderHasChildren(AssetBrowserNodeID id) const;

    void InjectPendingFoldersIntoProjection();
    static AssetBrowserNodeID MakePendingNodeID(const std::string& path);
    static bool IsPendingNodeID(AssetBrowserNodeID id);

    AssetBrowserNodeID GetRootNodeID(EditorHost& host) const;
};

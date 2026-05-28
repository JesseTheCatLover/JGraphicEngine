//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <unordered_set>
#include <vector>
#include <functional>

#include "Core/Delegates/FDelegateHandle.h"
#include "Core/Services/Types/FAssetBrowserViewState.h"
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
        std::vector<std::string> createOrder;
        std::unordered_set<std::string> createSet;
        std::unordered_map<std::string, std::vector<std::string>> childrenByParent;
        std::unordered_set<std::string> expanded;
        std::string renameTarget;
        std::string renameBuffer;

        bool bStartRenameFocus = false;
    };

    // Pending virtual folders
    FPendingFolderState m_Pending;

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

    void DrawContent(EditorHost& host, EditorRuntime& runtime);
    void DrawTopBar();
    void DrawDirectoryList();
    void DrawBottomBar(EditorHost& host);

    void SetCurrentPath(const std::string& path);

    void DrawDirectoryNodeByID(AssetBrowserNodeID id);

    void ExpandAll();
    void CollapseAll();

    void SyncPathInputToCurrentPath();
    bool ApplyPathInput(EditorHost& host);

    bool AddPendingFolderPath(const std::string& rawPath);
    void RemovePendingFolderPathAndDescendants(const std::string& normalizedPath);
    void RebuildPendingChildrenIndex();
    bool CommitPendingFolders(EditorHost& host);
    void DrawPendingChildRow(const std::string& childPath);
    bool CommitPendingRename(const std::string& oldPath, const std::string& newName);

    void EndPendingRename();
    void CancelPendingRename();

    [[nodiscard]] std::string MakeUniqueChildPath(const std::string& parent, const std::string& baseName) const;

    std::string OnCreateFolderClicked();

    static bool IsRootPath(const std::string& path);
    static bool IsSameOrUnderPath(const std::string& normalizedAncestor, const std::string& normalizedPath);
    [[nodiscard]] bool IsFolderPresentInUI(const std::string& normalizedPath) const;
    bool HasPendingChildFolder(const std::string& normalizedParent) const;
    bool FolderHasChildren(const FAssetBrowserNode& node) const;
};

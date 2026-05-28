// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "Core/IEditorService.h"
#include "Core/Delegates/TMulticastDelegate.h"
#include "FAssetBrowserNode.h"
#include "FAssetBrowserViewState.h"

struct FAssetOpResult;
class EditorFileAPI;
class SelectionService;
class EditorHost;

struct FSelectionModifiers;

class AssetBrowserService final : public IEditorService
{
private:
    EditorHost&   m_Host;
    EditorFileAPI& m_FileAPI;

    TMulticastDelegate<const FAssetOpResult&> m_OnAssetsMutated;

    struct FModelGraph
    {
        // stable identity
        std::unordered_map<std::string, AssetBrowserNodeID> pathToID;
        std::unordered_map<AssetBrowserNodeID, FAssetBrowserNode> nodes;

        // adjacency
        std::unordered_map<AssetBrowserNodeID, std::vector<AssetBrowserNodeID>> children;

        // folder state
        std::unordered_set<AssetBrowserNodeID> loadedFolders; // direct children are known
        std::unordered_set<AssetBrowserNodeID> dirtyFolders;  // needs re-list + rebuild its direct children

        AssetBrowserNodeID nextID = 1; // 0 reserved for "none"
    };

    FModelGraph m_Model;
    uint64_t m_GraphVersion = 1; // increments on changes

public:
    explicit AssetBrowserService(EditorHost& host, EditorFileAPI& fileAPI);

    // --- IEditorService ---
    void Tick(float deltaTime) override;
    void Shutdown() override;
    void RegisterShellCommands(ShellCommandService& shell) override;

    // --- View refresh ---
    void MarkDirty(FAssetBrowserViewState& view) { view.bDirty = true; }
    void RefreshView(FAssetBrowserViewState& view);

    // --- Node access helpers (operate on a view) ---
    [[nodiscard]] const FAssetBrowserNode* GetNode(const FAssetBrowserViewState& view, AssetBrowserNodeID id) const;
    [[nodiscard]] const FAssetBrowserNode* GetNodeByPath(const FAssetBrowserViewState& view, const std::string& path) const;

    // --- Selection helpers (respect view.selectionPolicy) ---
    void SelectPath(FAssetBrowserViewState& view, const std::string& virtualPath, bool bToggle = false, bool bRange = false);
    void SelectNode(FAssetBrowserViewState& view, const FAssetBrowserNode& node, bool bToggle = false, bool bRange = false);

    [[nodiscard]] bool IsPathSelected(const FAssetBrowserViewState& view, const std::string& virtualPath) const;
    [[nodiscard]] bool IsNodeSelected(const FAssetBrowserViewState& view, const FAssetBrowserNode& node) const;

    void ClearSelection(FAssetBrowserViewState& view);

    // Broadcast after mutations so open panels can refresh.
    TMulticastDelegate<const FAssetOpResult&>& OnAssetsMutated() { return m_OnAssetsMutated; }

    // --- Mutation Ops ---
    [[nodiscard]] FAssetOpResult CreateFolder(FAssetBrowserViewState& view, const std::string& folderVirtualPath);
    [[nodiscard]] FAssetOpResult DeleteFolder(FAssetBrowserViewState& view, const std::string& folderVirtualPath, bool bRecursive = true);
    [[nodiscard]] FAssetOpResult RenameFolder(FAssetBrowserViewState& view, const std::string& oldVirtualPath, const std::string& newVirtualPath);
    [[nodiscard]] FAssetOpResult MoveFolder(FAssetBrowserViewState& view, const std::string& sourceVirtualPath, const std::string& destVirtualPath);

    [[nodiscard]] FAssetOpResult DeleteAsset(FAssetBrowserViewState& view, const std::string& virtualAssetPath);
    [[nodiscard]] FAssetOpResult RenameAsset(FAssetBrowserViewState& view, const std::string& virtualAssetPath, const std::string& newName);
    [[nodiscard]] FAssetOpResult MoveAsset(FAssetBrowserViewState& view, const std::string& sourceVirtualAssetPath, const std::string& destVirtualFolder);
    [[nodiscard]] FAssetOpResult DuplicateAsset(FAssetBrowserViewState& view, const std::string& sourceVirtualAssetPath, const std::string& destVirtualAssetPath);

    // Multi-selection helpers (service-level convenience)
    [[nodiscard]] FAssetOpResult DeletePaths(FAssetBrowserViewState& view, const std::vector<std::string>& virtualPaths, bool bRecursiveFolders = true);
    [[nodiscard]] FAssetOpResult MovePathsToFolder(FAssetBrowserViewState& view, const std::vector<std::string>& sourceVirtualPaths, const std::string& destVirtualFolder);

private:
    TSelectionModel<std::string>& GetSelectionModel(FAssetBrowserViewState& view);
    [[nodiscard]] const TSelectionModel<std::string>& GetSelectionModel(const FAssetBrowserViewState& view) const;

    [[nodiscard]] uint64_t GetGraphVersion() const { return m_GraphVersion; }

    AssetBrowserNodeID EnsureNode(const std::string& virtualPath, EAssetBrowserNodeType type);
    AssetBrowserNodeID EnsureFolder(const std::string& folderVirtualPath);

    void LinkChild(AssetBrowserNodeID parent, AssetBrowserNodeID child);
    void UnlinkChild(AssetBrowserNodeID parent, AssetBrowserNodeID child);

    [[nodiscard]] const FAssetBrowserNode* TryGetModelNode(AssetBrowserNodeID id) const;
    [[nodiscard]] AssetBrowserNodeID TryGetID(const std::string& virtualPath) const;

    void MarkFolderDirtyByPath(const std::string& folderVirtualPath);
    void RefreshFolderDirectChildren(const std::string& folderVirtualPath);
    void RefreshFolderDirectChildrenByID(AssetBrowserNodeID folderID);

    void ApplyMutationToModelGraph(const FAssetOpResult& result);

    // Post-mutation UI reconciliation (selection translation + invalidation + broadcast)
    void PostMutation(FAssetBrowserViewState& initiatingView, const FAssetOpResult& result);

    static void MergeOpResult(FAssetOpResult& ioAgg, const FAssetOpResult& r);

    bool IsSameOrUnder(const std::string& folder, const std::string& candidate);
};

// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "Core/IEditorService.h"
#include "Core/Delegates/TMulticastDelegate.h"
#include "FAssetBrowserNode.h"
#include "FAssetBrowserViewProjection.h"

class AssetBrowserViewController;
struct FAssetOpResult;
class EditorFileAPI;
class SelectionService;
class EditorHost;

struct FSelectionModifiers;

template <typename T> class TSelectionModel;

class AssetBrowserService final : public IEditorService
{
private:
    friend class AssetBrowserProjectionBuilder;

    EditorHost& m_Host;
    EditorFileAPI& m_FileAPI;

    TMulticastDelegate<const FAssetOpResult&> m_OnAssetsMutated;

    struct FModelGraph
    {
        // stable identity
        std::unordered_map<AssetBrowserNodeID, FAssetBrowserNode> nodes;
        std::unordered_map<std::string, AssetBrowserNodeID> pathToID;

        // adjacency
        std::unordered_map<AssetBrowserNodeID, std::vector<AssetBrowserNodeID>> children;

        // folder state
        std::unordered_set<AssetBrowserNodeID> loadedFolders; // direct children are known
        std::unordered_set<AssetBrowserNodeID> dirtyFolders;  // needs re-list + rebuild its direct children

        AssetBrowserNodeID nextID = 1; // 0 reserved for "none"
    };

    FModelGraph m_Model;

public:
    explicit AssetBrowserService(EditorHost& host, EditorFileAPI& fileAPI);

    // --- IEditorService ---
    void Tick(float deltaTime) override;
    void Shutdown() override;
    void RegisterShellCommands(ShellCommandService& shell) override;

    // --- View refresh ---
    void RefreshView(const AssetBrowserViewController& controller, FAssetBrowserViewProjection& view);

    // --- Node access helpers (operate on a view) ---
    [[nodiscard]] const FAssetBrowserNode* GetNode(const FAssetBrowserViewProjection& view, AssetBrowserNodeID id) const;
    [[nodiscard]] const FAssetBrowserNode* GetNodeByPath(const FAssetBrowserViewProjection& view, const std::string& path) const;
    AssetBrowserNodeID GetRootNodeID(const std::string& path);
    FAssetBrowserNode* GetMutableNode(AssetBrowserNodeID id);
    [[nodiscard]] const std::vector<AssetBrowserNodeID>& GetChildren(AssetBrowserNodeID id) const;

    [[nodiscard]] std::vector<AssetBrowserNodeID>GetAllFolderIDs() const;

    void PurgeNodeSubtree(AssetBrowserNodeID rootID);

    void ProbeFolderChildStates(AssetBrowserNodeID folderID);

    // Broadcast after mutations so open panels can refresh.
    TMulticastDelegate<const FAssetOpResult&>& OnAssetsMutated() { return m_OnAssetsMutated; }

    // --- Global Selection ---

    [[nodiscard]] TSelectionModel<std::string>& GetGlobalSelectionModel();
    [[nodiscard]] const TSelectionModel<std::string>& GetGlobalSelectionModel() const;

    // --- Mutation Ops ---
    [[nodiscard]] FAssetOpResult CreateFolder(const std::string& folderVirtualPath);
    [[nodiscard]] FAssetOpResult DeleteFolder(const std::string& folderVirtualPath, bool bRecursive = true);
    [[nodiscard]] FAssetOpResult RenameFolder(const std::string& oldVirtualPath, const std::string& newVirtualPath);
    [[nodiscard]] FAssetOpResult MoveFolder(const std::string& sourceVirtualPath, const std::string& destVirtualPath);

    [[nodiscard]] FAssetOpResult DeleteAsset(const std::string& virtualAssetPath);
    [[nodiscard]] FAssetOpResult RenameAsset(const std::string& virtualAssetPath, const std::string& newName);
    [[nodiscard]] FAssetOpResult MoveAsset(const std::string& sourceVirtualAssetPath, const std::string& destVirtualFolder);
    [[nodiscard]] FAssetOpResult DuplicateAsset(const std::string& sourceVirtualAssetPath, const std::string& destVirtualAssetPath);

    // Multi-selection helpers (service-level convenience)
    [[nodiscard]] FAssetOpResult DeletePaths(const std::vector<std::string>& virtualPaths, bool bRecursiveFolders = true);
    [[nodiscard]] FAssetOpResult MovePathsToFolder(const std::vector<std::string>& sourceVirtualPaths, const std::string& destVirtualFolder);

private:
    FModelGraph& GetModelGraph() { return m_Model; }

    AssetBrowserNodeID EnsureNode(const std::string& virtualPath, EAssetBrowserNodeType type);
    AssetBrowserNodeID EnsureFolder(const std::string& folderVirtualPath);

    void EnsureFolderLoaded(AssetBrowserNodeID folderID);

    void LinkChild(AssetBrowserNodeID parent, AssetBrowserNodeID child);
    void UnlinkChild(AssetBrowserNodeID parent, AssetBrowserNodeID child);

    [[nodiscard]] const FAssetBrowserNode* TryGetModelNode(AssetBrowserNodeID id) const;
    [[nodiscard]] AssetBrowserNodeID TryGetID(const std::string& virtualPath) const;

    void MarkFolderDirtyByPath(const std::string& folderVirtualPath);

    std::string RemapPath(const std::string& path, const std::unordered_map<std::string,std::string>& remaps);

    void ApplyMutationToModelGraph(const FAssetOpResult& result);

    // Post-mutation UI reconciliation (selection translation + invalidation + broadcast)
    void PostMutation(const FAssetOpResult& result);

    static void MergeOpResult(FAssetOpResult& ioAgg, const FAssetOpResult& r);
};

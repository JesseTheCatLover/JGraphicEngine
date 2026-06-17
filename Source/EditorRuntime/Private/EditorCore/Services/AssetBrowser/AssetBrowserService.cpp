// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorCore/Services/AssetBrowser/AssetBrowserService.h"

#include <algorithm>
#include <unordered_set>

#include "EditorCore/Services/Selection/TSelectionModel.h"
#include "EditorCore/Services/AssetBrowser/AssetBrowserProjectionBuilder.h"
#include "EditorCore/EditorHost.h"
#include "EditorCore/Services/Selection/SelectionService.h"
#include "Assets/FAssetRecord.h"
#include "Utilities/UPath.h"

AssetBrowserService::AssetBrowserService(EditorHost& host, EditorFileAPI& fileAPI)
    : m_Host(host)
    , m_FileAPI(fileAPI)
{
}

void AssetBrowserService::Tick(float /*deltaTime*/)
{
    // Service does not own view lifetimes; panels decide when to call RefreshView(view).
}

void AssetBrowserService::Shutdown()
{
}

void AssetBrowserService::RegisterShellCommands(ShellCommandService& /*shell*/)
{
}

FAssetOpResult AssetBrowserService::CreateFolder(const std::string& folderVirtualPath)
{
    FAssetOpResult r = m_FileAPI.CreateFolder(folderVirtualPath);
    PostMutation(r);
    return r;
}

FAssetOpResult AssetBrowserService::DeleteFolder(const std::string& folderVirtualPath, bool bRecursive)
{
    FAssetOpResult r = m_FileAPI.DeleteFolder(folderVirtualPath, bRecursive);
    PostMutation(r);
    return r;
}

FAssetOpResult AssetBrowserService::RenameFolder(const std::string& oldVirtualPath, const std::string& newVirtualPath)
{
    FAssetOpResult r = m_FileAPI.RenameFolder(oldVirtualPath, newVirtualPath);
    PostMutation(r);
    return r;
}

FAssetOpResult AssetBrowserService::MoveFolder(const std::string& sourceVirtualPath, const std::string& destVirtualPath)
{
    FAssetOpResult r = m_FileAPI.MoveFolder(sourceVirtualPath, destVirtualPath);
    PostMutation(r);
    return r;
}

FAssetOpResult AssetBrowserService::DeleteAsset(const std::string& virtualAssetPath)
{
    FAssetOpResult r = m_FileAPI.DeleteAsset(virtualAssetPath);
    PostMutation(r);
    return r;
}

FAssetOpResult AssetBrowserService::RenameAsset(const std::string& virtualAssetPath, const std::string& newName)
{
    FAssetOpResult r = m_FileAPI.RenameAsset(virtualAssetPath, newName);
    PostMutation(r);
    return r;
}

FAssetOpResult AssetBrowserService::MoveAsset(const std::string& sourceVirtualAssetPath,
                                             const std::string& destVirtualFolder)
{
    FAssetOpResult r = m_FileAPI.MoveAsset(sourceVirtualAssetPath, destVirtualFolder);
    PostMutation(r);
    return r;
}

FAssetOpResult AssetBrowserService::DuplicateAsset(const std::string& sourceVirtualAssetPath, const std::string& destVirtualAssetPath)
{
    FAssetOpResult r = m_FileAPI.DuplicateAsset(sourceVirtualAssetPath, destVirtualAssetPath);
    PostMutation(r);
    return r;
}

FAssetOpResult AssetBrowserService::DeletePaths(const std::vector<std::string>& virtualPaths,
                                               bool bRecursiveFolders)
{
    FAssetOpResult agg;
    agg.bSuccess = true;

    for (const auto& raw : virtualPaths)
    {
        const std::string p = UPath::NormalizeVirtual(raw);

        // Decide asset vs folder using registry (best available signal here)
        if (m_FileAPI.FindAssetByVirtualPath(p))
        {
            MergeOpResult(agg, m_FileAPI.DeleteAsset(p));
        }
        else
        {
            MergeOpResult(agg, m_FileAPI.DeleteFolder(p, bRecursiveFolders));
        }
    }

    PostMutation(agg);
    return agg;
}

FAssetOpResult AssetBrowserService::MovePathsToFolder(const std::vector<std::string>& sourceVirtualPaths,
                                                     const std::string& destVirtualFolder)
{
    FAssetOpResult agg;
    agg.bSuccess = true;

    const std::string dstFolder = UPath::NormalizeVirtual(destVirtualFolder);

    for (const auto& raw : sourceVirtualPaths)
    {
        const std::string src = UPath::NormalizeVirtual(raw);

        if (m_FileAPI.FindAssetByVirtualPath(src))
        {
            MergeOpResult(agg, m_FileAPI.MoveAsset(src, dstFolder));
        }
        else
        {
            const std::string name = UPath::GetFileName(src);
            const std::string dstFolderPath = UPath::Join(dstFolder, name);

            if (UPath::IsSameOrUnder(src, dstFolderPath))
            {
                FAssetOpResult err;
                err.bSuccess = false;
                err.errors.push_back("Cannot move a folder into itself or its subfolder: " + src + " -> " + dstFolderPath);
                MergeOpResult(agg, err);
                continue;
            }

            MergeOpResult(agg, m_FileAPI.MoveFolder(src, dstFolderPath));
        }
    }

    PostMutation(agg);
    return agg;
}

AssetBrowserNodeID AssetBrowserService::EnsureNode(const std::string& rawPath, EAssetBrowserNodeType type)
{
    const std::string path = UPath::NormalizeVirtual(rawPath);

    auto it = m_Model.pathToID.find(path);
    if (it != m_Model.pathToID.end())
        return it->second;

    // Ensure parent folder exists (except for roots like "/Project")
    AssetBrowserNodeID parentID = 0;
    std::string parentPath;

    if (type == EAssetBrowserNodeType::Folder)
        parentPath = UPath::GetParent(path);
    else
        parentPath = UPath::GetParent(path); // asset parent folder

    if (!parentPath.empty() && parentPath != path && parentPath != "/")
    {
        // Parent is always a folder node
        parentID = EnsureNode(parentPath, EAssetBrowserNodeType::Folder);
    }

    AssetBrowserNodeID id = m_Model.nextID++;

    FAssetBrowserNode node;
    node.nodeID = id;
    node.parentID = parentID;
    node.type = type;
    node.virtualPath = path;
    node.displayName = UPath::GetFileName(path); // for folders, this is leaf folder name too

    // Root display name polish (optional)
    if (path == "/Project") node.displayName = "Project";
    if (path == "/Engine")  node.displayName = "Engine";

    m_Model.pathToID.emplace(node.virtualPath, id);
    m_Model.nodes.emplace(id, node);

    if (parentID != 0)
    {
        LinkChild(parentID, id);
        m_Model.dirtyFolders.insert(parentID); // child order may change
    }

    return id;
}

AssetBrowserNodeID AssetBrowserService::EnsureFolder(const std::string& folderVirtualPath)
{
    return EnsureNode(folderVirtualPath, EAssetBrowserNodeType::Folder);
}

void AssetBrowserService::LinkChild(AssetBrowserNodeID parent, AssetBrowserNodeID child)
{
    auto& vec = m_Model.children[parent];
    if (std::find(vec.begin(), vec.end(), child) == vec.end())
        vec.push_back(child);
}

void AssetBrowserService::UnlinkChild(AssetBrowserNodeID parent, AssetBrowserNodeID child)
{
    auto it = m_Model.children.find(parent);
    if (it == m_Model.children.end()) return;

    auto& vec = it->second;
    vec.erase(std::remove(vec.begin(), vec.end(), child), vec.end());
}


const FAssetBrowserNode* AssetBrowserService::TryGetModelNode(AssetBrowserNodeID id) const
{
    auto it = m_Model.nodes.find(id);
    return (it != m_Model.nodes.end()) ? &it->second : nullptr;
}

AssetBrowserNodeID AssetBrowserService::TryGetID(const std::string& virtualPath) const
{
    const std::string p = UPath::NormalizeVirtual(virtualPath);
    auto it = m_Model.pathToID.find(p);
    return (it != m_Model.pathToID.end()) ? it->second : 0;
}

void AssetBrowserService::MarkFolderDirtyByPath(const std::string& folderVirtualPath)
{
    const std::string f = UPath::NormalizeVirtual(folderVirtualPath);

    if (f.empty())
        return;

    AssetBrowserNodeID id = EnsureFolder(f);
    m_Model.dirtyFolders.insert(id);
}

void AssetBrowserService::EnsureFolderLoaded(AssetBrowserNodeID folderID)
{
    const FAssetBrowserNode* folderNode = TryGetModelNode(folderID);
    if (!folderNode)
        return;

    if (folderNode->type != EAssetBrowserNodeType::Folder)
        return;

    const bool bLoaded = m_Model.loadedFolders.contains(folderID);
    const bool bDirty = m_Model.dirtyFolders.contains(folderID);

    if (bLoaded && !bDirty)
    {
        return;
    }

    const std::string folderPath = folderNode->virtualPath;

    bool bCurrentFolderHasFolderChildren = false;
    bool bCurrentFolderHasAssetChildren = false;

    // 1) Ask backend for immediate children
    const auto entries = m_FileAPI.ListDirectory(folderPath);

    // Preserve previous child list so we can detect removals
    const auto oldChildrenIt = m_Model.children.find(folderID);

    const std::vector<AssetBrowserNodeID>* oldChildren = nullptr;

    if (oldChildrenIt != m_Model.children.end())
    {
        oldChildren = &oldChildrenIt->second;
    }

    // 2) Rebuild direct children vector
    std::vector<AssetBrowserNodeID> newChildren;
    newChildren.reserve(entries.size());

    for (const auto& entry : entries)
    {
        if (entry.type == FVirtualDirEntry::EType::Folder)
        {
            bCurrentFolderHasFolderChildren = true;
            AssetBrowserNodeID childID = EnsureNode(entry.virtualPath, EAssetBrowserNodeType::Folder);

            auto& child = m_Model.nodes[childID];

            child.parentID = folderID;
            child.displayName = entry.name;

            if (child.childFolderState == EAssetBrowserChildState::Unknown ||
                child.childAssetState == EAssetBrowserChildState::Unknown)
            {
                ProbeFolderChildStates(childID); // Checks the children folder states too
            }

            newChildren.push_back(childID);
        }
        else
        {
            bCurrentFolderHasAssetChildren = true;
            AssetBrowserNodeID AssetID = EnsureNode(entry.virtualPath, EAssetBrowserNodeType::Asset);

            auto& n = m_Model.nodes[AssetID];
            n.parentID = folderID;
            n.displayName = entry.name;
            n.childFolderState = EAssetBrowserChildState::None; // Assets have no children
            n.childAssetState = EAssetBrowserChildState::None;

            if (const FAssetRecord* rec = m_FileAPI.FindAssetByVirtualPath(entry.virtualPath))
            {
                n.assetID = rec->assetID;
                n.assetType = rec->assetType;
                n.domain = rec->domain;
            }

            newChildren.push_back(AssetID);
        }
    }

    // Detect nodes that disappeared from the directory
    std::unordered_set<AssetBrowserNodeID> newChildSet;
    newChildSet.reserve(newChildren.size());

    for (AssetBrowserNodeID id : newChildren)
    {
        newChildSet.insert(id);
    }

    std::vector<AssetBrowserNodeID> removedChildren;

    if (oldChildren)
    {
        for (AssetBrowserNodeID oldID : *oldChildren)
        {
            if (!newChildSet.contains(oldID))
            {
                removedChildren.push_back(oldID);
            }
        }
    }

    // 3) Replace children list
    for (AssetBrowserNodeID removedID : removedChildren)
    {
        PurgeNodeSubtree(removedID);
    }

    // 4) Sort (folders first, then name)
    auto& vec = m_Model.children[folderID];
    vec = std::move(newChildren);

    std::sort(vec.begin(), vec.end(),
        [&](AssetBrowserNodeID a, AssetBrowserNodeID b)
    {
        const FAssetBrowserNode* A = TryGetModelNode(a);
        const FAssetBrowserNode* B = TryGetModelNode(b);

        if (A->type != B->type)
        {
            return A->type == EAssetBrowserNodeType::Folder;
        }

        return A->displayName < B->displayName;
    });

    // 5) Mark the current folder node with children states
    {
        auto& self = m_Model.nodes[folderID];

        self.childFolderState = bCurrentFolderHasFolderChildren
                ? EAssetBrowserChildState::Present
                : EAssetBrowserChildState::None;

        self.childAssetState = bCurrentFolderHasAssetChildren
                ? EAssetBrowserChildState::Present
                : EAssetBrowserChildState::None;
    }

    m_Model.loadedFolders.insert(folderID);
    m_Model.dirtyFolders.erase(folderID);
}

std::string AssetBrowserService::RemapPath(
    const std::string& path,
    const std::unordered_map<std::string,std::string>& remaps)
{
    const std::string normalized = UPath::NormalizeVirtual(path);

    const std::string* bestOld = nullptr;
    const std::string* bestNew = nullptr;

    size_t bestLength = 0;

    for (const auto& [oldRoot,newRoot] : remaps)
    {
        if (!UPath::IsSameOrUnder(oldRoot, normalized))
            continue;

        if (oldRoot.size() > bestLength)
        {
            bestLength = oldRoot.size();
            bestOld = &oldRoot;
            bestNew = &newRoot;
        }
    }

    if (!bestOld)
        return normalized;

    if (normalized == *bestOld)
        return UPath::NormalizeVirtual(*bestNew);

    const std::string suffix =
        normalized.substr(bestOld->size());

    return UPath::NormalizeVirtual(*bestNew + suffix);
}

void AssetBrowserService::ApplyMutationToModelGraph(const FAssetOpResult& result)
{
    // Mark affected folders dirty
    for (const auto& f : result.affectedVirtualFolders)
    {
        MarkFolderDirtyByPath(f);
    }

    // Deleted paths: mark their parent dirty (and optionally purge nodes)
    for (const auto& p : result.deletedPaths)
    {
        const std::string np = UPath::NormalizeVirtual(p);

        MarkFolderDirtyByPath(UPath::GetParent(np));

        if (AssetBrowserNodeID id = TryGetID(p))
        {
            PurgeNodeSubtree(id);
        }
    }

    // Remaps: old parent + new parent dirty
    for (const auto& [oldP, newP] : result.pathRemappings)
    {
        MarkFolderDirtyByPath(UPath::GetParent(oldP));
        MarkFolderDirtyByPath(UPath::GetParent(newP));

        if (AssetBrowserNodeID id = TryGetID(oldP)) // Treat remap as delete
        {
            PurgeNodeSubtree(id);
        }
    }
}

// -------------------------
// View projection
// -------------------------

void AssetBrowserService::RefreshView(const AssetBrowserViewController& controller, FAssetBrowserViewProjection& view)
{
    view.Clear();
    AssetBrowserProjectionBuilder::Build(*this, controller, view);
}

// -------------------------
// Node access
// -------------------------

const FAssetBrowserNode* AssetBrowserService::GetNode(const FAssetBrowserViewProjection& view, AssetBrowserNodeID id) const
{
    auto it = view.nodeCache.find(id);
    return (it != view.nodeCache.end()) ? &it->second : nullptr;
}

const FAssetBrowserNode* AssetBrowserService::GetNodeByPath(const FAssetBrowserViewProjection& view, const std::string& path) const
{
    const std::string normalized = UPath::NormalizeVirtual(path);

    auto it = view.pathToID.find(normalized);
    if (it == view.pathToID.end())
        return nullptr;
    return GetNode(view, it->second);
}

AssetBrowserNodeID AssetBrowserService::GetRootNodeID(const std::string& path)
{
    return EnsureFolder(path);
}

FAssetBrowserNode* AssetBrowserService::GetMutableNode(AssetBrowserNodeID id)
{
    auto it = m_Model.nodes.find(id);
    return (it != m_Model.nodes.end())
        ? &it->second
        : nullptr;
}

const std::vector<AssetBrowserNodeID>& AssetBrowserService::GetChildren(AssetBrowserNodeID id) const
{
    static const std::vector<AssetBrowserNodeID> empty;

    auto it = m_Model.children.find(id);
    return (it != m_Model.children.end())
        ? it->second
        : empty;
}

std::vector<AssetBrowserNodeID> AssetBrowserService::GetAllFolderIDs() const
{
    std::vector<AssetBrowserNodeID> result;
    result.reserve(m_Model.nodes.size());

    for (const auto& [id, node] : m_Model.nodes)
    {
        if (node.type == EAssetBrowserNodeType::Folder)
            result.push_back(id);
    }

    return result;
}

void AssetBrowserService::PurgeNodeSubtree(AssetBrowserNodeID rootID)
{
    auto childIt = m_Model.children.find(rootID);

    if (childIt != m_Model.children.end())
    {
        auto children = childIt->second;

        for (AssetBrowserNodeID child : children)
            PurgeNodeSubtree(child);
    }

    auto nodeIt = m_Model.nodes.find(rootID);

    if (nodeIt != m_Model.nodes.end())
    {
        const auto& node = nodeIt->second;

        if (node.parentID != 0)
        {
            UnlinkChild(node.parentID, rootID);

            m_Model.dirtyFolders.insert(node.parentID);
        }

        m_Model.pathToID.erase(node.virtualPath);
        m_Model.loadedFolders.erase(rootID);
        m_Model.dirtyFolders.erase(rootID);
    }

    m_Model.children.erase(rootID);
    m_Model.nodes.erase(rootID);
}

void AssetBrowserService::ProbeFolderChildStates(AssetBrowserNodeID folderID)
{
    FAssetBrowserNode* node = GetMutableNode(folderID);

    if (!node)
        return;

    if (node->type != EAssetBrowserNodeType::Folder)
        return;

    const auto entries = m_FileAPI.ListDirectory(node->virtualPath);

    bool bHasFolders = false;
    bool bHasAssets = false;

    for (const auto& e : entries)
    {
        if (e.type == FVirtualDirEntry::EType::Folder)
            bHasFolders = true;
        else
            bHasAssets = true;

        if (bHasFolders && bHasAssets)
            break;
    }

    node->childFolderState = bHasFolders
            ? EAssetBrowserChildState::Present
            : EAssetBrowserChildState::None;

    node->childAssetState = bHasAssets
            ? EAssetBrowserChildState::Present
            : EAssetBrowserChildState::None;
}

// -------------------------
// Global Selection model routing
// -------------------------

TSelectionModel<std::string> & AssetBrowserService::GetGlobalSelectionModel()
{
    return m_Host.GetService<SelectionService>().GetAssetPathSelection();
}

const TSelectionModel<std::string> & AssetBrowserService::GetGlobalSelectionModel() const
{
    return m_Host.GetService<SelectionService>().GetAssetPathSelection();
}

void AssetBrowserService::PostMutation(const FAssetOpResult& result)
{
    const bool bAnyChange =
        !result.deletedPaths.empty() ||
        !result.pathRemappings.empty() ||
        !result.affectedVirtualFolders.empty();

    if (bAnyChange)
    {
        ApplyMutationToModelGraph(result);

        auto& selection =GetGlobalSelectionModel();

        const auto& currentSelection = selection.GetSelection();

        std::vector<std::string> next;
        next.reserve(currentSelection.size());

        std::unordered_set<std::string> deleted;
        deleted.reserve(result.deletedPaths.size());

        for (const auto& del : result.deletedPaths)
            deleted.insert(UPath::NormalizeVirtual(del));

        for (const std::string& item : currentSelection)
        {
            const std::string p = UPath::NormalizeVirtual(item);

            if (deleted.contains(p))
                continue;

            next.push_back(RemapPath(p, result.pathRemappings));
        }

        selection.SetSelection(std::move(next));
    }

    m_OnAssetsMutated.Broadcast(result);
}

void AssetBrowserService::MergeOpResult(FAssetOpResult& ioAgg, const FAssetOpResult& r)
{
    ioAgg.bSuccess = ioAgg.bSuccess && r.bSuccess;

    ioAgg.errors.insert(ioAgg.errors.end(), r.errors.begin(), r.errors.end());
    ioAgg.warnings.insert(ioAgg.warnings.end(), r.warnings.begin(), r.warnings.end());

    ioAgg.affectedVirtualFolders.insert(ioAgg.affectedVirtualFolders.end(),
        r.affectedVirtualFolders.begin(), r.affectedVirtualFolders.end());

    for (const auto& [oldP, newP] : r.pathRemappings)
        ioAgg.pathRemappings[UPath::NormalizeVirtual(oldP)] = UPath::NormalizeVirtual(newP);

    // De-dupe deleted paths to keep results tidy
    std::unordered_set<std::string> seen;
    seen.reserve(ioAgg.deletedPaths.size() + r.deletedPaths.size());

    for (const auto& d : ioAgg.deletedPaths) seen.insert(UPath::NormalizeVirtual(d));
    for (const auto& d : r.deletedPaths)
    {
        const std::string nd = UPath::NormalizeVirtual(d);
        if (seen.insert(nd).second)
            ioAgg.deletedPaths.push_back(nd);
    }
}

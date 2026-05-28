// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "AssetBrowserService.h"

#include <algorithm>
#include <unordered_set>

#include "AssetBrowserProjectionBuilder.h"
#include "Core/EditorHost.h"
#include "Core/Services/Selection/SelectionService.h"
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

// -------------------------
// Path helpers
// -------------------------

bool AssetBrowserService::IsSameOrUnder(const std::string& folder, const std::string& candidate)
{
    const std::string f = UPath::Normalize(folder);
    const std::string c = UPath::Normalize(candidate);

    if (f == c)
        return true;

    // Ensure "/Project" matches "/Project/..." but not "/ProjectX"
    if (c.size() <= f.size())
        return false;

    if (c.compare(0, f.size(), f) != 0)
        return false;

    return c[f.size()] == '/';
}

FAssetOpResult AssetBrowserService::CreateFolder(FAssetBrowserViewState& view, const std::string& folderVirtualPath)
{
    FAssetOpResult r = m_FileAPI.CreateFolder(folderVirtualPath);
    PostMutation(view, r);
    return r;
}

FAssetOpResult AssetBrowserService::DeleteFolder(FAssetBrowserViewState& view, const std::string& folderVirtualPath, bool bRecursive)
{
    FAssetOpResult r = m_FileAPI.DeleteFolder(folderVirtualPath, bRecursive);
    PostMutation(view, r);
    return r;
}

FAssetOpResult AssetBrowserService::RenameFolder(FAssetBrowserViewState& view, const std::string& oldVirtualPath, const std::string& newVirtualPath)
{
    FAssetOpResult r = m_FileAPI.RenameFolder(oldVirtualPath, newVirtualPath);
    PostMutation(view, r);
    return r;
}

FAssetOpResult AssetBrowserService::MoveFolder(FAssetBrowserViewState& view, const std::string& sourceVirtualPath, const std::string& destVirtualPath)
{
    FAssetOpResult r = m_FileAPI.MoveFolder(sourceVirtualPath, destVirtualPath);
    PostMutation(view, r);
    return r;
}

FAssetOpResult AssetBrowserService::DeleteAsset(FAssetBrowserViewState& view, const std::string& virtualAssetPath)
{
    FAssetOpResult r = m_FileAPI.DeleteAsset(virtualAssetPath);
    PostMutation(view, r);
    return r;
}

FAssetOpResult AssetBrowserService::RenameAsset(FAssetBrowserViewState& view, const std::string& virtualAssetPath, const std::string& newName)
{
    FAssetOpResult r = m_FileAPI.RenameAsset(virtualAssetPath, newName);
    PostMutation(view, r);
    return r;
}

FAssetOpResult AssetBrowserService::MoveAsset(FAssetBrowserViewState& view,
                                             const std::string& sourceVirtualAssetPath,
                                             const std::string& destVirtualFolder)
{
    FAssetOpResult r = m_FileAPI.MoveAsset(sourceVirtualAssetPath, destVirtualFolder);
    PostMutation(view, r);
    return r;
}

FAssetOpResult AssetBrowserService::DuplicateAsset(FAssetBrowserViewState& view, const std::string& sourceVirtualAssetPath, const std::string& destVirtualAssetPath)
{
    FAssetOpResult r = m_FileAPI.DuplicateAsset(sourceVirtualAssetPath, destVirtualAssetPath);
    PostMutation(view, r);
    return r;
}

FAssetOpResult AssetBrowserService::DeletePaths(FAssetBrowserViewState& view,
                                               const std::vector<std::string>& virtualPaths,
                                               bool bRecursiveFolders)
{
    FAssetOpResult agg;
    agg.bSuccess = true;

    for (const auto& raw : virtualPaths)
    {
        const std::string p = UPath::Normalize(raw);

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

    PostMutation(view, agg);
    return agg;
}

FAssetOpResult AssetBrowserService::MovePathsToFolder(FAssetBrowserViewState& view,
                                                     const std::vector<std::string>& sourceVirtualPaths,
                                                     const std::string& destVirtualFolder)
{
    FAssetOpResult agg;
    agg.bSuccess = true;

    const std::string dstFolder = UPath::Normalize(destVirtualFolder);

    for (const auto& raw : sourceVirtualPaths)
    {
        const std::string src = UPath::Normalize(raw);

        if (m_FileAPI.FindAssetByVirtualPath(src))
        {
            MergeOpResult(agg, m_FileAPI.MoveAsset(src, dstFolder));
        }
        else
        {
            const std::string name = UPath::GetFileName(src);
            const std::string dstFolderPath = UPath::Join(dstFolder, name);

            if (IsSameOrUnder(src, dstFolderPath))
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

    PostMutation(view, agg);
    return agg;
}

AssetBrowserNodeID AssetBrowserService::EnsureNode(const std::string& rawPath, EAssetBrowserNodeType type)
{
    const std::string path = UPath::Normalize(rawPath);

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
    const std::string p = UPath::Normalize(virtualPath);
    auto it = m_Model.pathToID.find(p);
    return (it != m_Model.pathToID.end()) ? it->second : 0;
}

void AssetBrowserService::MarkFolderDirtyByPath(const std::string& folderVirtualPath)
{
    const std::string f = UPath::Normalize(folderVirtualPath);
    if (f.empty()) return;

    AssetBrowserNodeID id = EnsureFolder(f);
    m_Model.dirtyFolders.insert(id);
}

void AssetBrowserService::RefreshFolderDirectChildren(const std::string& folderVirtualPath)
{
    const std::string folder = UPath::Normalize(folderVirtualPath);
    AssetBrowserNodeID folderID = EnsureFolder(folder);
    RefreshFolderDirectChildrenByID(folderID);
}

void AssetBrowserService::RefreshFolderDirectChildrenByID(AssetBrowserNodeID folderID)
{
    const FAssetBrowserNode* folderNode = TryGetModelNode(folderID);
    if (!folderNode) return;
    if (folderNode->type != EAssetBrowserNodeType::Folder) return;

    const std::string folderPath = folderNode->virtualPath;

    // 1) Ask backend for immediate children
    const auto entries = m_FileAPI.ListDirectory(folderPath);

    bool bHasChildFolders = false;
    bool bHasChildAssets  = false;

    // 2) Rebuild direct children vector
    std::vector<AssetBrowserNodeID> newChildren;
    newChildren.reserve(entries.size());

    for (const auto& e : entries)
    {
        if (e.type == FVirtualDirEntry::EType::Folder)
        {
            bHasChildFolders = true;

            AssetBrowserNodeID cid = EnsureNode(e.virtualPath, EAssetBrowserNodeType::Folder);
            auto& child = m_Model.nodes[cid];
            child.parentID = folderID;
            child.displayName = e.name;

            if (!child.bChildFoldersKnown)
            {
                const auto subEntries = m_FileAPI.ListDirectory(e.virtualPath);

                bool bChildHasFolders = false;
                for (const auto& sub : subEntries)
                {
                    if (sub.type == FVirtualDirEntry::EType::Folder)
                    {
                        bChildHasFolders = true;
                        break;
                    }
                }

                child.bChildFoldersKnown = true;
                child.bHasChildFolders   = bChildHasFolders;
            }

            newChildren.push_back(cid);
        }
        else
        {
            bHasChildAssets = true;

            AssetBrowserNodeID aid = EnsureNode(e.virtualPath, EAssetBrowserNodeType::Asset);
            auto& n = m_Model.nodes[aid];
            n.parentID = folderID;
            n.displayName = e.name;

            if (const FAssetRecord* rec = m_FileAPI.FindAssetByVirtualPath(e.virtualPath))
            {
                n.assetID   = rec->assetID;
                n.assetType = rec->assetType;
                n.domain    = rec->domain;
            }

            newChildren.push_back(aid);
        }
    }

    // 3) Replace children list
    m_Model.children[folderID] = std::move(newChildren);

    // 4) Sort (folders first, then name)
    auto& vec = m_Model.children[folderID];
    std::sort(vec.begin(), vec.end(), [&](AssetBrowserNodeID a, AssetBrowserNodeID b)
    {
        const FAssetBrowserNode& A = m_Model.nodes.at(a);
        const FAssetBrowserNode& B = m_Model.nodes.at(b);

        if (A.type != B.type) return A.type == EAssetBrowserNodeType::Folder;
        return A.displayName < B.displayName;
    });

    // 5) Mark folder node authoritative state
    {
        auto& self = m_Model.nodes[folderID];
        self.bChildFoldersKnown = true;
        self.bHasChildFolders = bHasChildFolders;

        self.bChildAssetsKnown = true;
        self.bHasChildAssets = bHasChildAssets;
    }

    m_Model.loadedFolders.insert(folderID);
    m_Model.dirtyFolders.erase(folderID);

    ++m_GraphVersion;
}

void AssetBrowserService::ApplyMutationToModelGraph(const FAssetOpResult& result)
{
    // Mark affected folders dirty
    for (const auto& f : result.affectedVirtualFolders)
        MarkFolderDirtyByPath(f);

    // Deleted paths: mark their parent dirty (and optionally purge nodes)
    for (const auto& p : result.deletedPaths)
    {
        const std::string np = UPath::Normalize(p);
        MarkFolderDirtyByPath(UPath::GetParent(np));
    }

    // Remaps: old parent + new parent dirty
    for (const auto& [oldP, newP] : result.pathRemappings)
    {
        MarkFolderDirtyByPath(UPath::GetParent(UPath::Normalize(oldP)));
        MarkFolderDirtyByPath(UPath::GetParent(UPath::Normalize(newP)));
    }
}

// -------------------------
// View projection
// -------------------------

void AssetBrowserService::RefreshView(FAssetBrowserViewState& view)
{
    view.currentPath = UPath::Normalize(view.currentPath);
    view.rootPath = UPath::Normalize(view.rootPath);

    view.nodeCache.clear();
    view.pathToID.clear();
    view.children.clear();
    view.viewNodeIDs.clear();
    view.visibleVirtualPaths.clear();

    AssetBrowserProjectionBuilder::Build(*this, view);

    view.bDirty = false;
}

// -------------------------
// Node access
// -------------------------

const FAssetBrowserNode* AssetBrowserService::GetNode(const FAssetBrowserViewState& view, AssetBrowserNodeID id) const
{
    auto it = view.nodeCache.find(id);
    return (it != view.nodeCache.end()) ? &it->second : nullptr;
}

const FAssetBrowserNode* AssetBrowserService::GetNodeByPath(const FAssetBrowserViewState& view, const std::string& path) const
{
    const std::string normalized = UPath::Normalize(path);

    auto it = view.pathToID.find(normalized);
    if (it == view.pathToID.end())
        return nullptr;
    return GetNode(view, it->second);
}

// -------------------------
// Selection model routing
// -------------------------

TSelectionModel<std::string>& AssetBrowserService::GetSelectionModel(FAssetBrowserViewState& view)
{
    if (view.selectionPolicy == EAssetBrowserSelectionPolicy::SharedGlobalSelection)
        return m_Host.GetService<SelectionService>().GetAssetPathSelection();

    if (!view.localSelectionModel)
        view.localSelectionModel = MakeUnique<TSelectionModel<std::string>>();

    return *view.localSelectionModel;
}

const TSelectionModel<std::string>& AssetBrowserService::GetSelectionModel(const FAssetBrowserViewState& view) const
{
    if (view.selectionPolicy == EAssetBrowserSelectionPolicy::SharedGlobalSelection)
        return m_Host.GetService<SelectionService>().GetAssetPathSelection();

    static TSelectionModel<std::string> s_Empty;
    return view.localSelectionModel ? *view.localSelectionModel : s_Empty;
}

// -------------------------
// Selection helpers
// -------------------------

void AssetBrowserService::SelectPath(FAssetBrowserViewState& view, const std::string& virtualPath, bool bToggle, bool bRange)
{
    auto& model = GetSelectionModel(view);

    FSelectionModifiers mods;
    mods.bToggle = bToggle;
    mods.bRange  = bRange;

    model.ApplyClick(UPath::Normalize(virtualPath), mods, &view.visibleVirtualPaths);
}

void AssetBrowserService::SelectNode(FAssetBrowserViewState& view, const FAssetBrowserNode& node, bool bToggle, bool bRange)
{
    SelectPath(view, node.virtualPath, bToggle, bRange);
}

bool AssetBrowserService::IsPathSelected(const FAssetBrowserViewState& view, const std::string& virtualPath) const
{
    return GetSelectionModel(view).IsSelected(UPath::Normalize(virtualPath));
}

bool AssetBrowserService::IsNodeSelected(const FAssetBrowserViewState& view, const FAssetBrowserNode& node) const
{
    return IsPathSelected(view, node.virtualPath);
}

void AssetBrowserService::ClearSelection(FAssetBrowserViewState& view)
{
    GetSelectionModel(view).Clear();
}

void AssetBrowserService::PostMutation(FAssetBrowserViewState& initiatingView, const FAssetOpResult& result)
{
    const bool bAnyChange =
        !result.deletedPaths.empty() ||
        !result.pathRemappings.empty() ||
        !result.affectedVirtualFolders.empty();

    if (bAnyChange)
    {
        ApplyMutationToModelGraph(result);

        auto& model = GetSelectionModel(initiatingView);

        const auto& cur = model.GetSelection();
        std::vector<std::string> next;
        next.reserve(cur.size());

        std::unordered_set<std::string> deleted;
        deleted.reserve(result.deletedPaths.size());
        for (const auto& del : result.deletedPaths)
            deleted.insert(UPath::Normalize(del));

        for (const std::string& item : cur)
        {
            const std::string p = UPath::Normalize(item);

            if (deleted.find(p) != deleted.end())
                continue;

            if (auto it = result.pathRemappings.find(p); it != result.pathRemappings.end())
                next.push_back(UPath::Normalize(it->second));
            else
                next.push_back(p);
        }

        model.SetSelection(std::move(next));
        initiatingView.bDirty = true;
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
        ioAgg.pathRemappings[UPath::Normalize(oldP)] = UPath::Normalize(newP);

    // De-dupe deleted paths to keep results tidy
    std::unordered_set<std::string> seen;
    seen.reserve(ioAgg.deletedPaths.size() + r.deletedPaths.size());

    for (const auto& d : ioAgg.deletedPaths) seen.insert(UPath::Normalize(d));
    for (const auto& d : r.deletedPaths)
    {
        const std::string nd = UPath::Normalize(d);
        if (seen.insert(nd).second)
            ioAgg.deletedPaths.push_back(nd);
    }
}

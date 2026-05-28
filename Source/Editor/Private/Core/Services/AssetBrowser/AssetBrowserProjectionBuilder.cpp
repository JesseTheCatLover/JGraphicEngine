//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "AssetBrowserProjectionBuilder.h"

#include <unordered_set>

#include "AssetBrowserService.h"
#include "Utilities/UPath.h"

void AssetBrowserProjectionBuilder::Build(AssetBrowserService& service, FAssetBrowserViewState& view)
{
    auto& model = service.GetModelGraph();

    std::unordered_set<AssetBrowserNodeID> projected;
    projected.reserve(256);

    auto EnsureFolderLoaded = [&](AssetBrowserNodeID folderID)
    {
        if (folderID == 0)
            return;

        if (model.dirtyFolders.contains(folderID) ||
            !model.loadedFolders.contains(folderID))
        {
            service.RefreshFolderDirectChildrenByID(folderID);
        }
    };

    auto TryProjectNode = [&](AssetBrowserNodeID id) -> bool
    {
        if (!projected.insert(id).second)
            return false;

        const FAssetBrowserNode* n = service.TryGetModelNode(id);
        if (!n)
            return false;

        if (!view.bShowFolders &&
            n->type == EAssetBrowserNodeType::Folder)
        {
            return false;
        }

        if (!view.bShowAssets &&
            n->type == EAssetBrowserNodeType::Asset)
        {
            return false;
        }

        if (!view.searchFilter.empty() &&
            n->displayName.find(view.searchFilter) == std::string::npos)
        {
            return false;
        }

        view.nodeCache[id] = *n;
        view.pathToID[n->virtualPath] = id;
        view.viewNodeIDs.push_back(id);
        view.visibleVirtualPaths.push_back(n->virtualPath);

        return true;
    };

    switch (view.projectionMode)
    {
        case EAssetBrowserProjectionMode::Flat:
        {
            const AssetBrowserNodeID folderID =
                service.EnsureFolder(view.currentPath);

            EnsureFolderLoaded(folderID);

            if (auto it = model.children.find(folderID);
                it != model.children.end())
            {
                for (AssetBrowserNodeID child : it->second)
                {
                    TryProjectNode(child);
                }
            }

            return;
        }

        case EAssetBrowserProjectionMode::Tree:
        {
            const AssetBrowserNodeID rootID =
                service.EnsureFolder(view.rootPath);

            EnsureFolderLoaded(rootID);

            TryProjectNode(rootID);

            if (auto it = model.children.find(rootID);
                it != model.children.end())
            {
                view.children[rootID] = it->second;

                for (AssetBrowserNodeID child : it->second)
                {
                    TryProjectNode(child);
                }
            }

            // Expanded folders (lazy)
            for (const std::string& raw : view.expandedFolderPaths)
            {
                const std::string p = UPath::Normalize(raw);

                const AssetBrowserNodeID fid =
                    service.EnsureFolder(p);

                EnsureFolderLoaded(fid);

                auto cit = model.children.find(fid);
                if (cit == model.children.end())
                    continue;

                view.children[fid] = cit->second;

                for (AssetBrowserNodeID cid : cit->second)
                {
                    TryProjectNode(cid);
                }
            }
        }
    }
}
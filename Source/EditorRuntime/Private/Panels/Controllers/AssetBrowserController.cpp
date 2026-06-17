//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Panels/Controllers/AssetBrowserController.h"

#include <algorithm>
#include <unordered_set>

#include "EditorRuntime.h"
#include "EditorCore/EditorHost.h"
#include "EditorAPI/File/FileAPI.h"
#include "Assets/FAssetRecord.h"
#include "Panels/Controllers/Inputs/FAssetBrowserPanelInput.h"
#include "Panels/Controllers/Outputs/FAssetBrowserOutput.h"
#include "Utilities/UPath.h"

AssetBrowserController::AssetBrowserController(PanelID id, EditorHost& host, EditorRuntime& runtime)
    : m_PanelID(id)
    , m_Host(host)
    , m_Runtime(runtime)
    , m_FileAPI(m_Runtime.GetFile())
{
    // Initialize default path
    m_Document.currentPath = "/Project";

    // Mark as dirty so first Refresh() builds it
    m_bDirty = true;
}

AssetBrowserController::~AssetBrowserController()
{
    // Nothing special yet.
    // If we later subscribe to registry events, unsubscribe here.
}

namespace
{
    static bool IsRootPath(const std::string& p)
    {
        return p == "/" || p.empty();
    }
}

void AssetBrowserController::Update(float /*deltaTime*/, const FAssetBrowserPanelInput &input, FAssetBrowserOutput &out)
{
    // Always reset out (Inspector does this too)
    out = {};
    out.bValid = true;

    // 1) Process navigation / commands
    if (input.bNavigateHome)
    {
        SetCurrentPath("/Project");
    }
    else if (input.bNavigateUp)
    {
        const std::string parent = ComputeParentPath(m_Document.currentPath);
        SetCurrentPath(parent);
    }
    else if (input.bNavigateToPath)
    {
        SetCurrentPath(input.navigateToPath);
    }

    if (input.bForceRefresh)
        m_bDirty = true;

    // 2) Rebuild doc if needed
    Refresh();

    // 3) Produce output snapshot
    out.document = m_Document;
}

void AssetBrowserController::OnPanelDestroyed()
{
    // Currently, nothing to clean up.
}

void AssetBrowserController::SetCurrentPath(const std::string& path)
{
    std::string normalized = UPath::NormalizeVirtual(path);

    if (m_Document.currentPath == normalized)
        return;

    m_Document.currentPath = normalized;
    m_bDirty = true;
}

void AssetBrowserController::Refresh()
{
    if (!m_bDirty) return;

    // Clear previous contents
    m_Document.directories.clear();
    m_Document.assets.clear();

    // Rebuild document.
    BuildDirectories();
    BuildAssets();

    m_bDirty = false;
}

void AssetBrowserController::BuildDirectories()
{
    // We base directories on user-visible assets, so hidden EnginePrivate assets
    // don’t create confusing folders in the UI.
    std::vector<const FAssetRecord*> assets = m_FileAPI.GetAllUserVisibleAssets();

    const std::string parentDir = UPath::NormalizeVirtual(m_Document.currentPath);

    // Use a set to avoid duplicate directory entries.
    std::unordered_set<std::string> seenDirVirtualPaths;
    seenDirVirtualPaths.reserve(32);

    for (const FAssetRecord* record : assets)
    {
        if (!record)
            continue;

        std::string childName;
        std::string childVirtualPath;

        if (!IsDirectChildDirectory(parentDir, record->virtualPath, childName, childVirtualPath))
            continue;

        if (!seenDirVirtualPaths.insert(childVirtualPath).second)
            continue; // already added

        FAssetBrowserDirectory dir;
        dir.name = std::move(childName);
        dir.virtualPath = std::move(childVirtualPath);

        m_Document.directories.emplace_back(std::move(dir));
    }

    // Optional: sort directories alphabetically.
    std::sort(m_Document.directories.begin(), m_Document.directories.end(),
              [](const FAssetBrowserDirectory& a, const FAssetBrowserDirectory& b)
              {
                  return a.name < b.name;
              });
}

void AssetBrowserController::BuildAssets()
{
    // Same source as directories: user-visible only.
    std::vector<const FAssetRecord*> assets = m_FileAPI.GetAllUserVisibleAssets();

    const std::string parentDir = UPath::NormalizeVirtual(m_Document.currentPath);

    for (const FAssetRecord* record : assets)
    {
        if (!record)
            continue;

        const std::string virtualPath = UPath::NormalizeVirtual(record->virtualPath);

        if (!IsDirectChildAsset(parentDir, virtualPath))
            continue;

        FAssetBrowserAsset browserAsset;
        browserAsset.name        = record->assetName.empty()
                                   ? virtualPath // fallback
                                   : record->assetName;
        browserAsset.virtualPath = virtualPath;
        browserAsset.assetID     = record->assetID;
        browserAsset.type        = record->assetType;
        browserAsset.domain      = record->domain;
        browserAsset.record      = record;

        m_Document.assets.emplace_back(std::move(browserAsset));
    }

    // Optional: sort assets alphabetically.
    std::sort(m_Document.assets.begin(), m_Document.assets.end(),
              [](const FAssetBrowserAsset& a, const FAssetBrowserAsset& b)
              {
                  return a.name < b.name;
              });
}

// ------------------------
// Helpers
// ------------------------

bool AssetBrowserController::IsDirectChildDirectory(
    const std::string& parentDir,
    const std::string& assetVirtualPathRaw,
    std::string& outChildDirName,
    std::string& outChildDirVirtualPath) const
{
    // Example:
    // parentDir: /Project
    // assetVirtualPath: /Project/Textures/Wood/Wood.jasset
    //
    // Directories we want:
    // /Project/Textures (childName=Textures)
    // NOT /Project/Textures/Wood (since that's a sub-subdirectory of /Project)

    if (assetVirtualPathRaw.empty())
        return false;

    const std::string assetVirtualPath = UPath::NormalizeVirtual(assetVirtualPathRaw);
    if (assetVirtualPath.empty() || assetVirtualPath[0] != '/')
        return false;

    std::string normalizedParent = UPath::NormalizeVirtual(parentDir);

    // If parent is "/", we accept anything starting with "/"
    if (normalizedParent == "/")
    {
        // assetVirtualPath: /Project/Textures/Wood.jasset
        // We want the first component after "/" -> "Project"
        // That corresponds to directory "/Project".

        // Find first slash after the leading one.
        std::size_t secondSlash = assetVirtualPath.find('/', 1);
        if (secondSlash == std::string::npos)
        {
            // No second slash: asset is like "/Foo.jasset" directly in root,
            // so there is no directory child.
            return false;
        }

        // Directory name is substring between the leading '/' and secondSlash.
        outChildDirName = assetVirtualPath.substr(1, secondSlash - 1);
        outChildDirVirtualPath = assetVirtualPath.substr(0, secondSlash);
        return true;
    }

    // For non-root parents:
    // parentDir: /Project
    // assetVirtualPath must start with "/Project/"
    std::string prefix = normalizedParent;
    prefix.push_back('/'); // "/Project/"

    if (assetVirtualPath.rfind(prefix, 0) != 0)
        return false;

    // Trim the parent prefix.
    // remainder: "Textures/Wood/Wood.jasset"
    std::string remainder = assetVirtualPath.substr(prefix.size());
    if (remainder.empty())
        return false;

    // Split remainder by '/'.
    std::size_t slashPos = remainder.find('/');
    if (slashPos == std::string::npos)
    {
        // remainder has no '/', so asset is directly under parentDir
        // e.g. assetVirtualPath = "/Project/Barrel.jasset"
        // This does NOT produce a directory.
        return false;
    }

    // First component between 0 and slashPos is the child directory name.
    outChildDirName = remainder.substr(0, slashPos);

    // Construct the child's virtual path: parentDir + "/" + childDirName
    outChildDirVirtualPath = normalizedParent;
    outChildDirVirtualPath.push_back('/');
    outChildDirVirtualPath += outChildDirName;

    return true;
}

bool AssetBrowserController::IsDirectChildAsset(
    const std::string& parentDir,
    const std::string& assetVirtualPathRaw) const
{
    // Example:
    // parentDir: /Project/Textures
    // assetVirtualPath: /Project/Textures/Wood.jasset -> direct child
    // assetVirtualPath: /Project/Textures/Wood/Wood.jasset -> NOT direct child

    if (assetVirtualPathRaw.empty())
        return false;

    const std::string assetVirtualPath = UPath::NormalizeVirtual(assetVirtualPathRaw);

    // Normalize parentDir.
    std::string normalizedParent = UPath::NormalizeVirtual(parentDir);

    // If parent is "/", then direct child assets are like "/Foo.jasset"
    if (normalizedParent == "/")
    {
        // Must be like "/Name.ext" with no additional '/'
        // after the leading one.
        if (assetVirtualPath.size() < 2)
            return false;

        if (assetVirtualPath[0] != '/')
            return false;

        // No additional '/' after index 1.
        std::size_t slashPos = assetVirtualPath.find('/', 1);
        return (slashPos == std::string::npos);
    }

    // For non-root parents:
    // assetVirtualPath must start with "/ParentDir/"
    std::string prefix = normalizedParent;
    prefix.push_back('/'); // e.g. "/Project/Textures/"

    if (assetVirtualPath.rfind(prefix, 0) != 0)
        return false;

    // remainder: e.g. "Wood.jasset" or "Wood/Wood.jasset"
    std::string remainder = assetVirtualPath.substr(prefix.size());
    if (remainder.empty())
        return false;

    // Direct child asset: remainder contains no '/'
    return remainder.find('/') == std::string::npos;
}

std::string AssetBrowserController::ComputeParentPath(const std::string& path)
{
    std::string p = UPath::NormalizeVirtual(path);
    if (IsRootPath(p))
        return "/";

    // Remove trailing slash if any (Normalize likely already does, but be safe)
    while (p.size() > 1 && p.back() == '/')
        p.pop_back();

    std::size_t lastSlash = p.find_last_of('/');
    if (lastSlash == std::string::npos || lastSlash == 0)
        return "/";

    return p.substr(0, lastSlash);
}

// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "AssetRegistrySubsystem.h"

#include <utility>
#include "Utilities/UPath.h"

void AssetRegistrySubsystem::Clear()
{
    m_Assets.clear();
    m_ByAssetID.clear();
    m_ByVirtualPath.clear();
    m_ByPhysicalPath.clear();
}

void AssetRegistrySubsystem::Shutdown()
{
    Clear();
}

const FAssetRecord* AssetRegistrySubsystem::FindAssetByAssetID(const std::string& assetID) const
{
    auto it = m_ByAssetID.find(assetID);
    if (it == m_ByAssetID.end())
        return nullptr;

    return &m_Assets[it->second];
}

const FAssetRecord* AssetRegistrySubsystem::FindAssetByVirtualPath(const std::string& virtualPath) const
{
    auto it = m_ByVirtualPath.find(virtualPath);
    if (it == m_ByVirtualPath.end())
        return nullptr;

    return &m_Assets[it->second];
}

std::vector<const FAssetRecord*> AssetRegistrySubsystem::FindAllAssetsByVirtualPathPrefix(
    const std::string& virtualPathPrefix) const
{
    std::vector<const FAssetRecord*> results;

    for (const FAssetRecord& record : m_Assets)
    {
        if (record.virtualPath.starts_with(virtualPathPrefix))
            results.push_back(&record);
    }

    return results;
}

const FAssetRecord* AssetRegistrySubsystem::FindAssetByPhysicalPath(const std::string& physicalPath) const
{
    auto it = m_ByPhysicalPath.find(UPath::Normalize(physicalPath));
    if (it == m_ByPhysicalPath.end())
        return nullptr;

    return &m_Assets[it->second];
}

std::vector<const FAssetRecord*> AssetRegistrySubsystem::GetAllUserVisibleAssets() const
{
    std::vector<const FAssetRecord*> results;

    for (const FAssetRecord& record : m_Assets)
    {
        if (record.visibility == EAssetVisibility::EnginePrivate)
            continue;

        results.push_back(&record);
    }

    return results;
}

std::vector<const FAssetRecord*> AssetRegistrySubsystem::GetAllAssetsByType(EAssetType type) const
{
    std::vector<const FAssetRecord*> results;

    for (const FAssetRecord& record : m_Assets)
    {
        if (record.assetType == type)
            results.push_back(&record);
    }

    return results;
}

std::vector<const FAssetRecord*> AssetRegistrySubsystem::GetAllAssetsByDomain(EAssetDomain domain) const
{
    std::vector<const FAssetRecord*> results;

    for (const FAssetRecord& record : m_Assets)
    {
        if (record.domain == domain)
            results.push_back(&record);
    }

    return results;
}

std::vector<const FAssetRecord*> AssetRegistrySubsystem::GetAllAssetsByVisibility(
    EAssetVisibility visibility) const
{
    std::vector<const FAssetRecord*> results;

    for (const FAssetRecord& record : m_Assets)
    {
        if (record.visibility == visibility)
            results.push_back(&record);
    }

    return results;
}

std::vector<const FAssetRecord*> AssetRegistrySubsystem::GetAllDependenciesForAsset(
    const std::string& assetID) const
{
    std::vector<const FAssetRecord*> results;

    const FAssetRecord* asset = FindAssetByAssetID(assetID);
    if (!asset)
        return results;

    for (const std::string& depID : asset->dependencyAssetIDs)
    {
        const FAssetRecord* dep = FindAssetByAssetID(depID);
        if (dep)
            results.push_back(dep);
    }

    return results;
}

// -------------------------
// Primitive mutations
// -------------------------

bool AssetRegistrySubsystem::RegisterAsset(const FAssetRecord& record)
{
    if (record.assetID.empty())
        return false;

    if (record.virtualPath.empty())
        return false;

    if (record.physicalPath.empty())
        return false;

    if (m_ByAssetID.contains(record.assetID))
        return false;

    if (m_ByVirtualPath.contains(record.virtualPath))
        return false;

    // Normalize physical path on ingest so FindAssetByPhysicalPath works reliably.
    const std::string normalizedPhysical = UPath::Normalize(record.physicalPath);

    if (m_ByPhysicalPath.contains(normalizedPhysical))
        return false;

    const size_t index = m_Assets.size();

    // Copy record, but ensure stored physical path is normalized.
    m_Assets.push_back(record);
    m_Assets[index].physicalPath = normalizedPhysical;

    m_ByAssetID.emplace(m_Assets[index].assetID, index);
    m_ByVirtualPath.emplace(m_Assets[index].virtualPath, index);
    m_ByPhysicalPath.emplace(m_Assets[index].physicalPath, index);

    return true;
}

bool AssetRegistrySubsystem::UnregisterAssetByVirtualPath(const std::string& virtualPath)
{
    auto it = m_ByVirtualPath.find(virtualPath);
    if (it == m_ByVirtualPath.end())
        return false;

    const size_t idx = it->second;
    const size_t last = m_Assets.size() - 1;

    if (idx != last)
        std::swap(m_Assets[idx], m_Assets[last]);

    m_Assets.pop_back();

    // Simpler + safe (can optimize later)
    RebuildIndices();
    return true;
}

bool AssetRegistrySubsystem::UnregisterAssetByAssetID(const std::string& assetID)
{
    auto it = m_ByAssetID.find(assetID);
    if (it == m_ByAssetID.end())
        return false;

    const size_t idx = it->second;
    const size_t last = m_Assets.size() - 1;

    if (idx != last)
        std::swap(m_Assets[idx], m_Assets[last]);

    m_Assets.pop_back();

    RebuildIndices();
    return true;
}

bool AssetRegistrySubsystem::UpdateAssetVirtualPath(const std::string& oldVirtualPath,
                                                    const std::string& newVirtualPath)
{
    if (oldVirtualPath.empty() || newVirtualPath.empty())
        return false;

    auto itOld = m_ByVirtualPath.find(oldVirtualPath);
    if (itOld == m_ByVirtualPath.end())
        return false;

    if (m_ByVirtualPath.contains(newVirtualPath))
        return false; // uniqueness invariant

    const size_t idx = itOld->second;
    m_Assets[idx].virtualPath = newVirtualPath;

    RebuildIndices();
    return true;
}

bool AssetRegistrySubsystem::UpdateAssetPhysicalPathByAssetID(const std::string& assetID,
                                                              const std::string& newPhysicalPath)
{
    if (assetID.empty() || newPhysicalPath.empty())
        return false;

    auto it = m_ByAssetID.find(assetID);
    if (it == m_ByAssetID.end())
        return false;

    const std::string normalized = UPath::Normalize(newPhysicalPath);

    // If another asset already uses this physical path, reject.
    auto itPhys = m_ByPhysicalPath.find(normalized);
    if (itPhys != m_ByPhysicalPath.end())
    {
        const size_t otherIdx = itPhys->second;
        if (m_Assets[otherIdx].assetID != assetID)
            return false;
    }

    const size_t idx = it->second;
    m_Assets[idx].physicalPath = normalized;

    RebuildIndices();
    return true;
}

bool AssetRegistrySubsystem::UpdateAssetMetaByAssetID(const std::string& assetID,
                                                      const FAssetRecord& newMeta)
{
    if (assetID.empty())
        return false;

    auto it = m_ByAssetID.find(assetID);
    if (it == m_ByAssetID.end())
        return false;

    const size_t idx = it->second;

    // Preserve identity + paths (registry invariants); update everything else.
    const std::string keepAssetID = m_Assets[idx].assetID;
    const std::string keepVirtual = m_Assets[idx].virtualPath;
    const std::string keepPhysical = m_Assets[idx].physicalPath;

    m_Assets[idx] = newMeta;

    m_Assets[idx].assetID = keepAssetID;
    m_Assets[idx].virtualPath = keepVirtual;
    m_Assets[idx].physicalPath = keepPhysical;

    RebuildIndices();
    return true;
}

// -------------------------
// Bulk replace
// -------------------------

FRegistryUpdateResult AssetRegistrySubsystem::ReplaceFolderContents(
    const std::string& folderVirtualPathRaw,
    const std::vector<FAssetRecord>& newRecords)
{
    FRegistryUpdateResult out;

    const std::string folder = NormalizeVirtualFolder(folderVirtualPathRaw);
    if (folder.empty())
    {
        out.AddFailure("", "ReplaceFolderContents: folderVirtualPath is empty");
        return out;
    }

    // 1) Remove anything currently in that folder
    const size_t before = m_Assets.size();

    std::vector<FAssetRecord> kept;
    kept.reserve(m_Assets.size());

    for (const FAssetRecord& r : m_Assets)
    {
        if (!IsInVirtualFolder(r.virtualPath, folder))
            kept.push_back(r);
    }

    m_Assets.swap(kept);
    RebuildIndices();

    out.removed = static_cast<int>(before - m_Assets.size());

    // 2) Register new records that belong to this folder
    for (const FAssetRecord& rec : newRecords)
    {
        if (!IsInVirtualFolder(rec.virtualPath, folder))
        {
            out.AddFailure(rec.virtualPath, "Record is not under target folder prefix");
            continue;
        }

        // RegisterAsset normalizes physical path & enforces uniqueness
        if (!RegisterAsset(rec))
        {
            out.AddFailure(rec.virtualPath, "RegisterAsset failed (duplicate ID/path or invalid record)");
            continue;
        }

        out.added++;
    }

    return out;
}

bool AssetRegistrySubsystem::DeleteAssetsInFolder(const std::string& folderVirtualPathRaw)
{
    const std::string folder = NormalizeVirtualFolder(folderVirtualPathRaw);
    if (folder.empty())
        return false;

    bool anyDeleted = false;

    std::vector<FAssetRecord> newAssets;
    newAssets.reserve(m_Assets.size());

    for (const FAssetRecord& record : m_Assets)
    {
        if (IsInVirtualFolder(record.virtualPath, folder))
        {
            anyDeleted = true;
            continue;
        }
        newAssets.push_back(record);
    }

    if (!anyDeleted)
        return false;

    m_Assets.swap(newAssets);
    RebuildIndices();
    return true;
}

// -------------------------
// Index rebuild
// -------------------------

void AssetRegistrySubsystem::RebuildIndices()
{
    m_ByAssetID.clear();
    m_ByVirtualPath.clear();
    m_ByPhysicalPath.clear();

    m_ByAssetID.reserve(m_Assets.size());
    m_ByVirtualPath.reserve(m_Assets.size());
    m_ByPhysicalPath.reserve(m_Assets.size());

    for (size_t i = 0; i < m_Assets.size(); ++i)
    {
        const FAssetRecord& r = m_Assets[i];
        m_ByAssetID[r.assetID] = i;
        m_ByVirtualPath[r.virtualPath] = i;
        m_ByPhysicalPath[r.physicalPath] = i;
    }
}

// -------------------------
// Helpers
// -------------------------

std::string AssetRegistrySubsystem::NormalizeVirtualFolder(std::string folderVirtualPath)
{
    // Normalize only what we must for consistent prefix checks:
    // - remove trailing slash (except if it's exactly "/")
    if (folderVirtualPath.size() > 1 && folderVirtualPath.back() == '/')
        folderVirtualPath.pop_back();

    return folderVirtualPath;
}

bool AssetRegistrySubsystem::IsInVirtualFolder(const std::string& assetVirtualPath,
                                               const std::string& folderVirtualPath)
{
    if (folderVirtualPath.empty())
        return false;

    const std::string folder = NormalizeVirtualFolder(folderVirtualPath);

    if (assetVirtualPath == folder)
        return true;

    const std::string prefix = folder + "/";
    return assetVirtualPath.rfind(prefix, 0) == 0; // starts_with(prefix)
}
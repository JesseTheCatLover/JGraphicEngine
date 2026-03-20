// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "AssetRegistrySubsystem.h"

#include <iostream>

#include "Assets/AssetFile.h"
#include "Core/Project/VirtualPathMounter.h"
#include "Utilities/UFileSystem.h"
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

bool AssetRegistrySubsystem::Rebuild(const VirtualPathMounter& mounter)
{
    Clear();

    bool bOk = true;

    if (mounter.IsMounted("/Engine"))
        bOk &= ScanMount(mounter, "/Engine");

    if (mounter.IsMounted("/Project"))
        bOk &= ScanMount(mounter, "/Project");

    return bOk;
}

bool AssetRegistrySubsystem::ScanMount(const VirtualPathMounter& mounter, const std::string& virtualRoot)
{
    const FVirtualMountPoint* mount = mounter.FindMount(virtualRoot);
    if (!mount)
        return false;

    const std::string& physicalRoot = mount->physicalRoot;

    if (!UFileSystem::DirectoryExists(physicalRoot))
    {
        std::cerr << "[AssetRegistry]: Mounted root does not exist: " << physicalRoot << "\n";
        return false;
    }

    const std::vector<std::string> files =
        UFileSystem::ListFiles(physicalRoot, "jasset", true, true);

    bool bAllGood = true;

    for (const std::string& physicalPath : files)
    {
        FAssetHeader header;

        if (!AssetFile::ReadHeader(physicalPath, header))
        {
            std::cerr << "[AssetRegistry]: Failed to read asset header: " << physicalPath << "\n";
            bAllGood = false;
            continue;
        }

        std::string virtualPath;

        if (!mounter.ResolvePhysicalToVirtual(physicalPath, virtualPath))
        {
            std::cerr << "[AssetRegistry]: Failed to convert physical path to virtual path: "
                      << physicalPath << "\n";
            bAllGood = false;
            continue;
        }

        FAssetRecord record;

        record.assetID = header.assetID;
        record.assetName = header.assetName;
        record.assetType = header.assetType;
        record.encoding = header.encoding;
        record.containerVersion = header.containerVersion;
        record.payloadVersion = header.payloadVersion;
        record.virtualPath = virtualPath;
        record.physicalPath = UPath::Normalize(physicalPath);
        record.sourcePath = header.sourcePath;
        record.importerName = header.importerName;
        record.dependencyAssetIDs = header.dependencyAssetIDs;

        DetermineDomainAndVisibility(record, virtualRoot);

        if (!RegisterAsset(std::move(record)))
        {
            std::cerr << "[AssetRegistry]: Duplicate or invalid asset registration for: "
                      << physicalPath << "\n";
            bAllGood = false;
            continue;
        }
    }

    return bAllGood;
}

void AssetRegistrySubsystem::DetermineDomainAndVisibility( // TODO: maybe for future the header of assets can override the visibility
    FAssetRecord& record,
    const std::string& virtualRoot) const
{
    if (virtualRoot == "/Engine")
        record.domain = EAssetDomain::Engine;
    else
        record.domain = EAssetDomain::Project;

    if (record.domain == EAssetDomain::Project)
    {
        record.visibility = EAssetVisibility::Project;
        return;
    }

    if (record.virtualPath.starts_with("/Engine/Editor"))
        record.visibility = EAssetVisibility::EnginePrivate;
    else
        record.visibility = EAssetVisibility::EnginePublic;
}

const FAssetRecord* AssetRegistrySubsystem::FindByAssetID(const std::string& assetID) const
{
    auto it = m_ByAssetID.find(assetID);
    if (it == m_ByAssetID.end())
        return nullptr;

    return &m_Assets[it->second];
}

const FAssetRecord* AssetRegistrySubsystem::FindByVirtualPath(const std::string& virtualPath) const
{
    auto it = m_ByVirtualPath.find(virtualPath);
    if (it == m_ByVirtualPath.end())
        return nullptr;

    return &m_Assets[it->second];
}

const FAssetRecord* AssetRegistrySubsystem::FindByPhysicalPath(const std::string& physicalPath) const
{
    auto it = m_ByPhysicalPath.find(UPath::Normalize(physicalPath));
    if (it == m_ByPhysicalPath.end())
        return nullptr;

    return &m_Assets[it->second];
}

std::vector<const FAssetRecord*> AssetRegistrySubsystem::GetUserVisibleAssets() const
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

std::vector<const FAssetRecord*> AssetRegistrySubsystem::GetAssetsByPrefix(
    const std::string& virtualPrefix) const
{
    std::vector<const FAssetRecord*> results;

    for (const FAssetRecord& record : m_Assets)
    {
        if (record.virtualPath.starts_with(virtualPrefix))
            results.push_back(&record);
    }

    return results;
}

std::vector<const FAssetRecord*> AssetRegistrySubsystem::GetAssetsByType(EAssetType type) const
{
    std::vector<const FAssetRecord*> results;

    for (const FAssetRecord& record : m_Assets)
    {
        if (record.assetType == type)
            results.push_back(&record);
    }

    return results;
}

std::vector<const FAssetRecord*> AssetRegistrySubsystem::GetAssetsByDomain(EAssetDomain domain) const
{
    std::vector<const FAssetRecord*> results;

    for (const FAssetRecord& record : m_Assets)
    {
        if (record.domain == domain)
            results.push_back(&record);
    }

    return results;
}

std::vector<const FAssetRecord*> AssetRegistrySubsystem::GetAssetsByVisibility(
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

std::vector<const FAssetRecord*> AssetRegistrySubsystem::GetDependencies(
    const std::string& assetID) const
{
    std::vector<const FAssetRecord*> results;

    const FAssetRecord* asset = FindByAssetID(assetID);
    if (!asset)
        return results;

    for (const std::string& depID : asset->dependencyAssetIDs)
    {
        const FAssetRecord* dep = FindByAssetID(depID);
        if (dep)
            results.push_back(dep);
    }

    return results;
}

bool AssetRegistrySubsystem::RegisterAsset(FAssetRecord record)
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

    if (m_ByPhysicalPath.contains(record.physicalPath))
        return false;

    const size_t index = m_Assets.size();
    m_Assets.push_back(std::move(record));

    m_ByAssetID.emplace(m_Assets[index].assetID, index);
    m_ByVirtualPath.emplace(m_Assets[index].virtualPath, index);
    m_ByPhysicalPath.emplace(m_Assets[index].physicalPath, index);

    return true;
}

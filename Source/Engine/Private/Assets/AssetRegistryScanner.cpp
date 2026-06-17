// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "AssetRegistryScanner.h"

#include <iostream>

#include "Assets/AssetFile.h"
#include "Core/Project/VirtualPathMounter.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

FAssetScanResult AssetRegistryScanner::ScanRoot(const VirtualPathMounter& mounter, const std::string& virtualRoot) const
{
    FAssetScanResult result;

    const FVirtualMountPoint* mount = mounter.FindMount(virtualRoot);
    if (!mount)
    {
        result.bSuccess = false;
        result.issues.push_back({"", "[AssetScanner]: Root is not mounted: " + virtualRoot});
        return result;
    }

    const std::string& physicalRoot = mount->physicalRoot;

    if (!UFileSystem::DirectoryExists(physicalRoot))
    {
        result.bSuccess = false;
        result.issues.push_back({physicalRoot, "[AssetScanner]: Mounted root does not exist"});
        return result;
    }

    const std::vector<std::string> files =
        UFileSystem::ListFiles(physicalRoot, "jasset", true, true);

    result.records.reserve(files.size());

    for (const std::string& physicalPath : files)
    {
        FAssetHeader header;
        if (!AssetFile::ReadHeader(physicalPath, header))
        {
            result.bSuccess = false;
            result.issues.push_back({physicalPath, "[AssetScanner]: Failed to read asset header"});
            continue;
        }

        std::string virtualPath;
        if (!mounter.ResolvePhysicalToVirtual(physicalPath, virtualPath))
        {
            result.bSuccess = false;
            result.issues.push_back({physicalPath, "[AssetScanner]: Failed to convert physical path to virtual path"});
            continue;
        }

        FAssetRecord record;
        record.assetID = header.assetID;
        record.assetType = header.assetType;
        record.encoding = header.encoding;
        record.containerVersion = header.containerVersion;
        record.payloadVersion = header.payloadVersion;
        record.virtualPath = virtualPath;
        record.physicalPath = UPath::NormalizeVirtual(physicalPath);
        record.sourcePath = header.sourcePath;
        record.importerName = header.importerName;
        record.dependencyAssetIDs = header.dependencyAssetIDs;

        FillRecordWithDomainAndVisibility(record, virtualRoot);

        result.records.push_back(std::move(record));
    }

    return result;
}

FAssetScanResult AssetRegistryScanner::ScanFolder(const VirtualPathMounter& mounter, const std::string& folderVirtualPath) const
{
    FAssetScanResult result;

    std::string physicalFolder;
    if (!mounter.ResolveVirtualToPhysical(folderVirtualPath, physicalFolder))
    {
        result.bSuccess = false;
        result.issues.push_back({"", "[AssetScanner]: Failed to resolve virtual folder to physical: " + folderVirtualPath});
        return result;
    }

    if (!UFileSystem::DirectoryExists(physicalFolder))
    {
        // Not a hard error: nothing to scan.
        return result;
    }

    const std::vector<std::string> files =
        UFileSystem::ListFiles(physicalFolder, "jasset", true, true);

    result.records.reserve(files.size());

    for (const std::string& physicalPath : files)
    {
        FAssetHeader header;
        if (!AssetFile::ReadHeader(physicalPath, header))
        {
            result.bSuccess = false;
            result.issues.push_back({physicalPath, "[AssetScanner]: Failed to read asset header"});
            continue;
        }

        std::string virtualPath;
        if (!mounter.ResolvePhysicalToVirtual(physicalPath, virtualPath))
        {
            result.bSuccess = false;
            result.issues.push_back({physicalPath, "[AssetScanner]: Failed to convert physical path to virtual path"});
            continue;
        }

        FAssetRecord record;
        record.assetID = header.assetID;
        record.assetType = header.assetType;
        record.encoding = header.encoding;
        record.containerVersion = header.containerVersion;
        record.payloadVersion = header.payloadVersion;
        record.virtualPath = virtualPath;
        record.physicalPath = UPath::NormalizeVirtual(physicalPath);
        record.sourcePath = header.sourcePath;
        record.importerName = header.importerName;
        record.dependencyAssetIDs = header.dependencyAssetIDs;

        const std::string root = ExtractRootFromVirtualPath(virtualPath);
        FillRecordWithDomainAndVisibility(record, root);

        result.records.push_back(std::move(record));
    }

    return result;
}

void AssetRegistryScanner::FillRecordWithDomainAndVisibility(FAssetRecord& record, const std::string& virtualRoot)
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

    if (record.virtualPath.starts_with("/Engine/Editor/"))
        record.visibility = EAssetVisibility::EnginePrivate;
    else
        record.visibility = EAssetVisibility::EnginePublic;
}

std::string AssetRegistryScanner::ExtractRootFromVirtualPath(const std::string& virtualPath)
{
    if (virtualPath.empty() || virtualPath[0] != '/')
        return {};

    const std::size_t secondSlash = virtualPath.find('/', 1);
    if (secondSlash == std::string::npos)
        return virtualPath;

    return virtualPath.substr(0, secondSlash);
}

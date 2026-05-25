// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

#include "Assets/FAssetRecord.h"

class VirtualPathMounter;

/**
 * @brief Represents a non-fatal or fatal error encountered during scanning.
 */
struct FAssetScanIssue
{
    std::string physicalPath;
    std::string message;
};

/**
 * @brief Contains the results of a disk scan operation.
 */
struct FAssetScanResult
{
    /**
     * @brief True if everything scanned perfectly.
     * False if ANY issue occurred (e.g., a single unreadable file).
     *
     * NOTE: If false, `records` may STILL contain successfully scanned assets!
     * Always process `records` even if bOk is false, unless the array is empty.
     */
    bool bSuccess = true;

    /** @brief Successfully parsed asset records ready for the registry. */
    std::vector<FAssetRecord> records;

    /** @brief List of errors/warnings encountered during the scan. */
    std::vector<FAssetScanIssue> issues;
};

/**
 * @brief Stateless utility class responsible for reading the filesystem,
 * parsing asset headers, and generating FAssetRecords.
 *
 * This bridges the gap between disk (UFileSystem) and memory (AssetRegistrySubsystem).
 */
class AssetRegistryScanner
{
public:
    AssetRegistryScanner() = default;

    /**
     * @brief Scans an entire mounted virtual root (e.g., "/Engine" or "/Project").
     *
     * @param mounter The mounter used to resolve paths.
     * @param virtualRoot The virtual root path to scan.
     * @return FAssetScanResult containing discovered records and any issues.
     */
    FAssetScanResult ScanRoot(const VirtualPathMounter& mounter, const std::string& virtualRoot) const;

    /**
     * @brief Scans a specific virtual folder (e.g., "/Project/Textures").
     *
     * @param mounter The mounter used to resolve paths.
     * @param folderVirtualPath The virtual folder path to scan.
     * @return FAssetScanResult containing discovered records and any issues.
     */
    FAssetScanResult ScanFolder(const VirtualPathMounter& mounter, const std::string& folderVirtualPath) const;

private:
    /**
     * @brief Helper to process a single physical file into an FAssetRecord.
     * @return True if parsing succeeded, false if an issue occurred.
     */
    bool TryScanSingleFile(const std::string& physicalPath,
                           const VirtualPathMounter& mounter,
                           FAssetRecord& outRecord,
                           std::vector<FAssetScanIssue>& outIssues) const;

    /**
     * @brief Derives and assigns EAssetDomain and EAssetVisibility based on the virtual root/path.
     */
    static void FillRecordWithDomainAndVisibility(FAssetRecord& record, const std::string& virtualRoot);

    /**
     * @brief Extracts the root mount point from a full virtual path (e.g., "/Engine/Foo" -> "/Engine").
     */
    static std::string ExtractRootFromVirtualPath(const std::string& virtualPath);
};

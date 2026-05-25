// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "RegistryUpdateResult.h"
#include "Assets/FAssetRecord.h"

/**
 * @brief Pure in-memory index of all known assets.
 *
 * Responsibilities:
 * - Store asset records.
 * - Provide fast lookups by common keys (assetID, virtualPath, physicalPath).
 * - Provide simple query helpers
 * - Apply mutations requested by higher-level systems (AssetManager, watchers, etc.).
 *
 * Lifetime / pointer stability:
 * - Any pointer returned from query functions points into an internal vector.
 * - These pointers are INVALIDATED by any mutation (register/unregister/update/replace),
 *   because the underlying vector may reallocate and indices may change.
 * - Treat returned pointers as short-lived; prefer stable keys (assetID / paths) externally.
 */
class AssetRegistrySubsystem
// TODO:
// - Add secondary indices (by type/domain/visibility, sorted-by-virtual-path for prefix queries).
// - Add AssetRegistryCache (serialize/deserialize) for fast startup.
// - Add cook manifest/runtime registry generation.
{
private:
    std::vector<FAssetRecord> m_Assets;

    std::unordered_map<std::string, size_t> m_ByAssetID;
    std::unordered_map<std::string, size_t> m_ByVirtualPath;
    std::unordered_map<std::string, size_t> m_ByPhysicalPath;

public:
    AssetRegistrySubsystem() = default;
    ~AssetRegistrySubsystem() = default;

    AssetRegistrySubsystem(const AssetRegistrySubsystem&) = delete;
    AssetRegistrySubsystem& operator=(const AssetRegistrySubsystem&) = delete;
    AssetRegistrySubsystem(AssetRegistrySubsystem&&) = delete;
    AssetRegistrySubsystem& operator=(AssetRegistrySubsystem&&) = delete;

    /**
     * @brief Clears all records and indices.
     */
    void Clear();

    /**
     * @brief Shuts down the registry subsystem.
     *
     * Currently equivalent to Clear(), but kept for uniform subsystem lifecycle.
     */
    void Shutdown();

    /**
     * @brief Finds an asset by stable asset ID.
     * @return Pointer to record or nullptr if not found.
     *
     * Pointer is invalidated by any subsequent mutation.
     */
    [[nodiscard]] const FAssetRecord* FindAssetByAssetID(const std::string& assetID) const;

    /**
     * @brief Finds an asset by virtual path.
     * @return Pointer to record or nullptr if not found.
     *
     * Pointer is invalidated by any subsequent mutation.
     */
    [[nodiscard]] const FAssetRecord* FindAssetByVirtualPath(const std::string& virtualPath) const;

    /**
     * @brief Returns all assets under a virtual path prefix.
     *
     * Notes:
     * - This is currently O(N) over all assets.
     * - Prefix semantics should be treated as folder semantics:
     *   "/Game/Foo" matches "/Game/Foo" and "/Game/Foo/Bar", but not "/Game/Foobar".
     *
     * Pointer results are invalidated by any subsequent mutation.
     */
    [[nodiscard]] std::vector<const FAssetRecord*> FindAllAssetsByVirtualPathPrefix(
        const std::string& virtualPathPrefix) const;

    /**
     * @brief Finds an asset by physical path.
     * @return Pointer to record or nullptr if not found.
     *
     * Physical path lookups should use normalized paths.
     * Pointer is invalidated by any subsequent mutation.
     */
    [[nodiscard]] const FAssetRecord* FindAssetByPhysicalPath(const std::string& physicalPath) const;

    /**
     * @brief Returns the raw dense storage of all asset records.
     *
     * WARNING:
     * - References are invalidated by any mutation (vector reallocation / swaps).
     * - Prefer query functions if you need lookup by key.
     */
    [[nodiscard]] const std::vector<FAssetRecord>& GetAllAssets() const { return m_Assets; }

    /**
     * @brief Returns assets visible to the user (filters EnginePrivate).
     *
     * Notes:
     * - Currently O(N).
     * - Returned pointers are invalidated by any mutation.
     */
    [[nodiscard]] std::vector<const FAssetRecord*> GetAllUserVisibleAssets() const;

    /**
     * @brief Returns assets of a specific type.
     *
     * Notes:
     * - Currently O(N).
     * - Returned pointers are invalidated by any mutation.
     */
    [[nodiscard]] std::vector<const FAssetRecord*> GetAllAssetsByType(EAssetType type) const;

    /**
     * @brief Returns assets belonging to a specific domain.
     *
     * Notes:
     * - Currently O(N).
     * - Returned pointers are invalidated by any mutation.
     */
    [[nodiscard]] std::vector<const FAssetRecord*> GetAllAssetsByDomain(EAssetDomain domain) const;

    /**
     * @brief Returns assets with a specific visibility.
     *
     * Notes:
     * - Currently O(N).
     * - Returned pointers are invalidated by any mutation.
     */
    [[nodiscard]] std::vector<const FAssetRecord*> GetAllAssetsByVisibility(EAssetVisibility visibility) const;

    /**
     * @brief Returns dependency asset records for the specified asset.
     *
     * Behavior:
     * - If the asset does not exist, returns empty.
     * - If a dependency ID is not found in the registry, it is skipped.
     *
     * Notes:
     * - Complexity is O(D) lookups where D is number of dependency IDs.
     * - Returned pointers are invalidated by any mutation.
     */
    [[nodiscard]] std::vector<const FAssetRecord*> GetAllDependenciesForAsset(const std::string& assetID) const;

    // ---- Primitive mutations ----

    /**
     * @brief Register a single asset record.
     *
     * Requirements:
     * - record.assetID, record.virtualPath, record.physicalPath must be non-empty.
     * - record.assetID must be unique.
     * - record.virtualPath must be unique.
     * - record.physicalPath must be unique (physical path is expected to be normalized on ingest).
     *
     * @return True on success, false if validation failed or any uniqueness invariant is violated.
     */
    bool RegisterAsset(const FAssetRecord& record);

    /**
     * @brief Removes a single asset by virtual path.
     *
     * @return True if an asset existed and was removed, false otherwise.
     *
     * Notes:
     * - This compacts internal storage; indices and iteration order may change.
     */
    bool UnregisterAssetByVirtualPath(const std::string& virtualPath);

    /**
     * @brief Removes a single asset by asset ID.
     *
     * @return True if an asset existed and was removed, false otherwise.
     *
     * Notes:
     * - This compacts internal storage; indices and iteration order may change.
     */
    bool UnregisterAssetByAssetID(const std::string& assetID);

    /**
     * @brief Updates an asset's virtual path after a rename/move on disk.
     *
     * Requirements:
     * - oldVirtualPath must exist.
     * - newVirtualPath must be non-empty and must not already exist in the registry.
     *
     * @return True on success, false if old path doesn't exist or new path collides.
     */
    bool UpdateAssetVirtualPath(const std::string& oldVirtualPath,
                                const std::string& newVirtualPath);

    /**
     * @brief Updates an asset's physical path after a rename/move on disk.
     *
     * Requirements:
     * - assetID must exist.
     * - newPhysicalPath must be non-empty.
     * - newPhysicalPath must not be used by another asset.
     *
     * @return True on success, false otherwise.
     */
    bool UpdateAssetPhysicalPathByAssetID(const std::string& assetID,
                                          const std::string& newPhysicalPath);

    /**
     * @brief Updates metadata for an asset (typically after reimport / header refresh).
     *
     * Contract:
     * - The target asset is identified by assetID.
     * - assetID + virtualPath + physicalPath are treated as identity and should be preserved.
     *
     * @return True on success, false if assetID doesn't exist.
     */
    bool UpdateAssetMetaByAssetID(const std::string& assetID,
                                  const FAssetRecord& newMeta);

    // ---- Bulk replace (used for folder rebuilds, watcher coalescing, imports) ----

    /**
     * @brief Replace all registry entries under folderVirtualPath with newRecords.
     *
     * Use cases:
     * - Folder rebuild after scanning disk.
     * - Watcher reconciliation (out-of-band changes).
     * - Bulk import/refresh.
     *
     * Behavior (current implementation intent):
     * - Removes all existing records whose virtualPath is under folderVirtualPath.
     * - Attempts to register each record in newRecords that belongs under the same folder.
     * - Returns a structured result describing adds/removals/failures.
     *
     * Notes:
     * - The operation may be partially applied if some newRecords fail to register
     *   (unless you later make it atomic).
     */
    FRegistryUpdateResult ReplaceFolderContents(const std::string& folderVirtualPath,
                                                const std::vector<FAssetRecord>& newRecords);

    /**
     * @brief Delete all assets whose virtualPath is under the given folder prefix.
     *
     * @return True if any asset was deleted, false if nothing matched.
     *
     * Notes:
     * - This only affects the in-memory registry.
     * - Deleting files on disk should be handled by AssetManager.
     */
    bool DeleteAssetsInFolder(const std::string& folderVirtualPath);

private:
    /**
     * @brief Rebuilds all primary indices from m_Assets.
     *
     * Correctness-first implementation:
     * - Called after mutations that may invalidate indices.
     * - Can be optimized later with incremental updates.
     */
    void RebuildIndices();

    /**
     * @brief Normalizes a virtual folder string for consistent prefix checks.
     *
     * Expected behavior:
     * - Removes trailing '/' (except for root "/").
     * - Does not validate mount points or existence.
     */
    static std::string NormalizeVirtualFolder(std::string folderVirtualPath);

    /**
     * @brief Tests whether assetVirtualPath is equal to folderVirtualPath or within it.
     *
     * Folder semantics:
     * - "/A/B" contains "/A/B" and "/A/B/C"
     * - "/A/B" does NOT contain "/A/BB"
     */
    static bool IsInVirtualFolder(const std::string& assetVirtualPath,
                                  const std::string& folderVirtualPath);
};

// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <string>

#include "Assets/FAssetImportRequest.h"
#include "Assets/FAssetImportResult.h"
#include "Core/Memory/SmartPointers.h"

class IAssetImporter;
class VirtualPathMounter;

/**
 * @brief Subsystem responsible for routing source files (e.g. .png, .fbx)
 * to the appropriate IAssetImporter to produce cooked .jasset files.
 */
class AssetImportSubsystem
{
private:
    /** @brief List of registered asset importers. */
    std::vector<TUniquePtr<IAssetImporter>> m_Importers;

public:
    AssetImportSubsystem() = default;
    ~AssetImportSubsystem() = default;

    // Prevent copying and moving (Subsystems are singletons/managed instances)
    AssetImportSubsystem(const AssetImportSubsystem&) = delete;
    AssetImportSubsystem& operator=(const AssetImportSubsystem&) = delete;
    AssetImportSubsystem(AssetImportSubsystem&&) = delete;
    AssetImportSubsystem& operator=(AssetImportSubsystem&&) = delete;

    /**
     * @brief Registers the default built-in importers (e.g., Texture, Model).
     * Usually called during engine startup.
     */
    void RegisterEssentialImporters();

    /**
     * @brief Registers a custom asset importer.
     * Takes ownership of the importer pointer.
     */
    void RegisterImporter(TUniquePtr<IAssetImporter> importer);

    /**
     * @brief Removes all registered importers.
     */
    void ClearImporters();

    /**
     * @brief Cleans up the subsystem on engine exit.
     */
    void Shutdown();

    /**
     * @brief Main entry point for importing a source file.
     *
     * Validates the request, finds the correct importer based on file extension,
     * and delegates the actual import process to it.
     *
     * @param request The import configuration (source path, destination, settings).
     * @param pathMounter Mounter used to resolve virtual destinations to physical paths.
     * @param outResult Populated with success state, errors, and generated virtual paths.
     * @return True if the import succeeded, false otherwise.
     */
    bool Import(const FAssetImportRequest& request, // TODO: Implement a pre-pass format sniffing (Magic byte sniffing)
                const VirtualPathMounter& pathMounter,
                FAssetImportResult& outResult) const;

private:
    /**
     * @brief Looks up a compatible importer for the given file extension.
     * @param extension The file extension (can be with or without leading dot).
     * @return Pointer to the importer, or nullptr if none support the extension.
     */
    [[nodiscard]] const IAssetImporter* FindImporterForExtension(const std::string& extension) const;
};

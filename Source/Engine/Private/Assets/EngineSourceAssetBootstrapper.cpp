//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EngineSourceAssetBootstrapper.h"

#include "Framework/AssetManager.h"
#include "Assets/FAssetImportRequest.h"
#include "Assets/FAssetImportResult.h"
#include "Core/Project/ProjectContext.h"

#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

bool EngineSourceAssetBootstrapper::Bootstrap(AssetManager& assetManager,
                                         const ProjectContext& context)
{
    const std::string sourceAssetPath = UPath::Join(context.GetEngineRoot(), "SourceAssets");

    if (!UFileSystem::DirectoryExists(sourceAssetPath))
        UFileSystem::CreateDirectory(sourceAssetPath);

    const std::vector<std::string> files =
        UFileSystem::ListFiles(sourceAssetPath, "", true, true);

    bool bAllGood = true;

    for (const std::string& file : files)
    {
        if (!ProcessSourceFile(assetManager, context, file))
            bAllGood = false;
    }

    return bAllGood;
}

bool EngineSourceAssetBootstrapper::ProcessSourceFile(
    AssetManager& assetManager,
    const ProjectContext& context,
    const std::string& sourcePath)
{
    const std::string extension = UPath::GetExtension(sourcePath);
    if (extension.empty())
        return true;

    const std::string engineSourceRoot =
        UPath::Normalize(UPath::Join(context.GetEngineRoot(), "SourceAssets"));

    const std::string engineAssetsRoot =
        UPath::Normalize(UPath::Join(context.GetEngineRoot(), "Assets"));

    const std::string normalizedSource = UPath::Normalize(sourcePath);

    // Ensure file is inside Engine/SourceAssets
    if (!normalizedSource.starts_with(engineSourceRoot))
        return false;

    // Get relative path inside SourceAssets
    std::string relative = normalizedSource.substr(engineSourceRoot.length());
    if (!relative.empty() && relative[0] == '/')
        relative.erase(0, 1);

    const std::string relativeFolder = UPath::GetParent(relative);
    const std::string filenameNoExt = UPath::GetFileName(relative, false);

    // Virtual destination folder
    std::string destinationVirtualFolder = "/Engine";
    if (!relativeFolder.empty())
        destinationVirtualFolder = UPath::Join("/Engine", relativeFolder);

    // Compiled asset physical path
    std::string compiledPath = engineAssetsRoot;
    if (!relativeFolder.empty())
        compiledPath = UPath::Join(compiledPath, relativeFolder);

    compiledPath = UPath::Join(compiledPath, filenameNoExt + ".jasset");

    bool shouldImport = false;

    if (!UFileSystem::FileExists(compiledPath))
    {
        shouldImport = true;
    }
    else
    {
        const auto sourceTime = UFileSystem::GetLastWriteTime(normalizedSource);
        const auto compiledTime = UFileSystem::GetLastWriteTime(compiledPath);

        if (sourceTime > compiledTime)
            shouldImport = true;
    }

    if (!shouldImport)
        return true;

    FAssetImportRequest request;
    request.sourceFilePath = normalizedSource;
    request.destinationVirtualFolder = destinationVirtualFolder;

    FAssetImportResult result;
    return assetManager.ImportAsset(request, result);
}

//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Assets/Importers/AssetImporterBase.h"
#include <filesystem>
#include <algorithm>
#include "Core/Project/VirtualPathMounter.h"
#include "Utilities/UPath.h"

bool AssetImporterBase::Import(const FAssetImportRequest& request, const VirtualPathMounter& pathMounter,
                               FAssetImportResult& outResult) const
{
    outResult = {};

    if (!ValidateSource(request, outResult))
        return false;

    if (!ValidateDestination(request, outResult))
        return false;

    if (!ValidateExtension(request.sourceFilePath, GetSupportedSourceExtensions(), outResult))
    {
        return false;
    }

    std::string virtualPath;
    std::string physicalPath;

    if (!ResolveDestinationPaths(request, pathMounter, virtualPath, physicalPath, outResult))
    {
        return false;
    }

    return OnImport(request, pathMounter, virtualPath, physicalPath, outResult);
}

bool AssetImporterBase::ValidateSource(const FAssetImportRequest& request, FAssetImportResult& outResult) const
{
    if (request.sourceFilePath.empty())
    {
        outResult.errors.emplace_back("Source file path is empty.");
        return false;
    }

    if (!std::filesystem::exists(request.sourceFilePath))
    {
        outResult.errors.emplace_back("Source file does not exist: " + request.sourceFilePath);
        return false;
    }

    if (!std::filesystem::is_regular_file(request.sourceFilePath))
    {
        outResult.errors.emplace_back("Source path is not a file: " + request.sourceFilePath);
        return false;
    }

    return true;
}

bool AssetImporterBase::ValidateDestination(const FAssetImportRequest& request, FAssetImportResult& outResult) const
{
    if (request.destinationVirtualFolder.empty())
    {
        outResult.errors.emplace_back("Destination virtual folder is empty.");
        return false;
    }

    if (!IsSupportedImportRoot(request.destinationVirtualFolder))
    {
        outResult.errors.emplace_back("Destination must be under /Project or /Engine.");
        return false;
    }

    return true;
}

bool AssetImporterBase::ValidateExtension(const std::string& sourceFile, const std::vector<std::string>& supportedExtensions,
    FAssetImportResult& outResult) const
{
    std::filesystem::path path(sourceFile);

    std::string ext = ToLowerCopy(path.extension().string());

    if (!ext.empty() && ext[0] == '.')
        ext.erase(0, 1);

    for (const std::string& supported : supportedExtensions)
    {
        if (ext == ToLowerCopy(supported))
            return true;
    }

    outResult.errors.emplace_back("Unsupported source extension: " + ext);

    return false;
}

bool AssetImporterBase::ResolveDestinationPaths(const FAssetImportRequest& request, const VirtualPathMounter& pathMounter,
    std::string& outVirtualPath, std::string& outPhysicalPath, FAssetImportResult& outResult) const
{
    outVirtualPath = MakeDestinationVirtualPath(request.destinationVirtualFolder, request.sourceFilePath);

    if (!pathMounter.ResolveVirtualToPhysical(outVirtualPath, outPhysicalPath))
    {
        outResult.errors.emplace_back(
            "Failed to resolve destination path: " + outVirtualPath
        );
        return false;
    }

    if (std::filesystem::exists(outPhysicalPath) && !request.bOverwrite)
    {
        outResult.errors.emplace_back(
            "Asset already exists and overwrite is disabled: " + outVirtualPath
        );
        return false;
    }

    std::filesystem::path parent = std::filesystem::path(outPhysicalPath).parent_path();

    std::error_code ec;
    std::filesystem::create_directories(parent, ec);

    if (ec)
    {
        outResult.errors.emplace_back(
            "Failed to create destination directory: " + parent.string()
        );
        return false;
    }

    return true;
}

std::string AssetImporterBase::MakeDestinationVirtualPath(const std::string& destinationVirtualFolder,
    const std::string& sourceFilePath)
{
    const std::string sourceStem = UPath::GetFileName(sourceFilePath, /*bIncludeExtension*/false);
    return UPath::Join(destinationVirtualFolder, sourceStem + ".jasset");
}

bool AssetImporterBase::IsSupportedImportRoot(const std::string& virtualPath)
{
    return virtualPath.rfind("/Project", 0) == 0
            || virtualPath.rfind("/Engine", 0) == 0;
}

std::string AssetImporterBase::ToLowerCopy(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });

    return s;
}
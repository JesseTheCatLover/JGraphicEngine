//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "IAssetImporter.h"

class AssetImporterBase : public IAssetImporter
{
public:
    bool Import(const FAssetImportRequest& request, const VirtualPathMounter& pathMounter,
        FAssetImportResult& outResult) const final;

protected:
    virtual bool OnImport(const FAssetImportRequest& request, const VirtualPathMounter& pathMounter, const std::string& destinationVirtualPath,
        const std::string& destinationPhysicalPath, FAssetImportResult& outResult) const = 0;

private:
    // Helpers
    bool ValidateSource(const FAssetImportRequest& request, FAssetImportResult& outResult) const;

    bool ValidateDestination(const FAssetImportRequest& request, FAssetImportResult& outResult) const;

    bool ValidateExtension(const std::string& sourceFile, const std::vector<std::string>& supportedExtensions,
        FAssetImportResult& outResult) const;


    bool ResolveDestinationPaths(const FAssetImportRequest& request, const VirtualPathMounter& pathMounter,
        std::string& outVirtualPath, std::string& outPhysicalPath, FAssetImportResult& outResult) const;

    static std::string MakeDestinationVirtualPath(const std::string& destinationVirtualFolder,
        const std::string& sourceFilePath);

    static bool IsSupportedImportRoot(const std::string& virtualPath);

    static std::string ToLowerCopy(std::string s);
};
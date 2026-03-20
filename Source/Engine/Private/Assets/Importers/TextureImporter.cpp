//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Assets/Importers/TextureImporter.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <vector>
#include <stb/stb_image.h>

#include "Assets/AssetFile.h"
#include "Assets/FAssetHeader.h"
#include "Assets/Payloads/FTexturePayloadHeader.h"
#include "Utilities/UPath.h"
#include "Utilities/UUUID.h"

namespace
{
    static std::vector<uint8_t> BuildTexturePayloadRGBA8(int width,
                                                         int height,
                                                         int channels,
                                                         bool bSRGB,
                                                         const unsigned char* pixelData,
                                                         size_t pixelByteCount)
    {
        FTexturePayloadHeader header{};
        header.version = 1;
        header.width = static_cast<uint32_t>(width);
        header.height = static_cast<uint32_t>(height);
        header.channels = static_cast<uint32_t>(channels);
        header.bSRGB = bSRGB ? 1 : 0;
        header.pixelDataSize = static_cast<uint64_t>(pixelByteCount);

        std::vector<uint8_t> bytes(sizeof(FTexturePayloadHeader) + pixelByteCount);
        std::memcpy(bytes.data(), &header, sizeof(FTexturePayloadHeader));

        if (pixelByteCount > 0)
        {
            std::memcpy(bytes.data() + sizeof(FTexturePayloadHeader), pixelData, pixelByteCount);
        }

        return bytes;
    }
}

std::vector<std::string> TextureImporter::GetSupportedSourceExtensions() const
{
    return { "png", "jpg", "jpeg" };
}

bool TextureImporter::OnImport(const FAssetImportRequest &request, const VirtualPathMounter &pathMounter,
    const std::string &destinationVirtualPath, const std::string &destinationPhysicalPath,
    FAssetImportResult &outResult) const
{
    // ------------------------------------------------------------
    // Load source image
    // ------------------------------------------------------------
    stbi_set_flip_vertically_on_load(1);

    int width = 0;
    int height = 0;
    int sourceChannels = 0;

    unsigned char* pixels = stbi_load(request.sourceFilePath.c_str(), &width, &height,
        &sourceChannels, 4 // force RGBA8
    );

    if (!pixels)
    {
        outResult.errors.emplace_back("Failed to decode source texture image: " + request.sourceFilePath);
        return false;
    }

    if (width <= 0 || height <= 0)
    {
        stbi_image_free(pixels);
        outResult.errors.emplace_back("Decoded texture has invalid dimensions.");
        return false;
    }

    const size_t pixelByteCount =
        static_cast<size_t>(width) *
        static_cast<size_t>(height) * 4u;

    if (pixelByteCount == 0)
    {
        stbi_image_free(pixels);
        outResult.errors.emplace_back("Texture produced empty pixel buffer.");
        return false;
    }

    // ------------------------------------------------------------
    // Build payload
    // ------------------------------------------------------------
    const bool bSRGB = true;

    std::vector<uint8_t> payloadBytes =
        BuildTexturePayloadRGBA8(width, height, 4, bSRGB, pixels, pixelByteCount);

    stbi_image_free(pixels);
    pixels = nullptr;

    // ------------------------------------------------------------
    // Build asset header
    // ------------------------------------------------------------
    FAssetHeader header{};
    header.assetID = UUUID::GenerateUUID();
    header.assetName = UPath::GetFileName(request.sourceFilePath, false);
    header.assetType = EAssetType::Texture2D;
    header.encoding = EAssetEncoding::Binary;
    header.containerVersion = FAssetHeader::CurrentContainerVersion;
    header.payloadVersion = 1;
    header.sourcePath = UPath::Normalize(request.sourceFilePath);
    header.importerName = GetImporterName();
    header.dependencyAssetIDs.clear();

    // ------------------------------------------------------------
    // Write asset
    // ------------------------------------------------------------
    if (!AssetFile::WriteBinaryAsset(destinationPhysicalPath, header, payloadBytes))
    {
        outResult.errors.emplace_back("Failed to write imported texture asset file: " + destinationPhysicalPath);
        return false;
    }

    // ------------------------------------------------------------
    // Fill result
    // ------------------------------------------------------------
    FImportedAssetInfo created{};
    created.assetID = header.assetID;
    created.assetType = EAssetType::Texture2D;
    created.virtualPath = destinationVirtualPath;
    created.physicalPath = destinationPhysicalPath;

    outResult.createdAssets.push_back(std::move(created));
    outResult.bSuccess = true;

    return true;
}
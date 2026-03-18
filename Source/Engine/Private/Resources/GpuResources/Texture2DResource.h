//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

#include "Rendering/RHandles.h"
#include "Rendering/RObjects.h"
#include "Resources/GpuResource.h"

class AssetRegistrySubsystem;

class Texture2DResource : public GpuResource
{
public:
    struct FDesc
    {
        std::string assetId;  // Asset UUID registered in AssetRegistrySubsystem (.jasset)
        bool bSRGB = true;
        bool bGenerateMipmaps = true;
        ETexWrap wrapS = ETexWrap::Repeat;
        ETexWrap wrapT = ETexWrap::Repeat;
        ETexFilter minFilter = ETexFilter::LinearMipmapLinear;
        ETexFilter magFilter = ETexFilter::Linear;
    };

public:
    explicit Texture2DResource(FDesc desc, AssetRegistrySubsystem* assetRegistry);

    void OnCreateGpuResources() override;
    void OnDestroyGpuResources() override;

    [[nodiscard]] RTextureHandle GetTexture() const { return m_Texture; }
    [[nodiscard]] const std::string& GetAssetId() const { return m_Desc.assetId; }

private:
    void LoadCPU();
    void UploadGPU();
    void ReleaseCPU();

private:
    FDesc m_Desc;
    AssetRegistrySubsystem* m_AssetRegistry = nullptr;

    // CPU staging
    int m_W = 0;
    int m_H = 0;
    std::vector<unsigned char> m_Pixels;
    bool m_CpuReady = false;

    // GPU resource handle
    RTextureHandle m_Texture{};
};

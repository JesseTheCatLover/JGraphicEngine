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
        std::string assetID;
        bool bSRGB = true;
        bool bGenerateMipmaps = true;
        ETexWrap wrapS = ETexWrap::Repeat;
        ETexWrap wrapT = ETexWrap::Repeat;
        ETexFilter minFilter = ETexFilter::LinearMipmapLinear;
        ETexFilter magFilter = ETexFilter::Linear;
    };

public:
    explicit Texture2DResource(FDesc desc, AssetRegistrySubsystem* registry);

    bool OnCreateGpuResources() override;
    void OnDestroyGpuResources() override;

    [[nodiscard]] RTextureHandle GetTexture() const { return m_Texture; }
    [[nodiscard]] const std::string& GetAssetID() const { return m_Desc.assetID; }

private:
    void LoadCPU();
    bool UploadGPU();
    void ReleaseCPU();

private:
    FDesc m_Desc;

    // CPU staging
    int m_W = 0;
    int m_H = 0;
    std::vector<unsigned char> m_Pixels;
    bool m_CpuReady = false;

    // GPU resource
    RTextureHandle m_Texture{};
};

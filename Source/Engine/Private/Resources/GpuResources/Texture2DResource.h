//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.
// Texture2DResource.h
#pragma once
#include <string>
#include <vector>

#include "Rendering/RHandles.h"
#include "Rendering/RObjects.h"
#include "Resources/GpuResource.h"

class Texture2DResource : public GpuResource
{
public:
    struct FDesc
    {
        std::string path;     // abs or project-relative (e.g. "Assets/Editor/Icons/Move.png")
        bool bSRGB = true;
        bool bGenerateMipmaps = true;
        bool bFlipY = true;
        ETexWrap wrapS = ETexWrap::Repeat;
        ETexWrap wrapT = ETexWrap::Repeat;
        ETexFilter minFilter = ETexFilter::LinearMipmapLinear;
        ETexFilter magFilter = ETexFilter::Linear;
    };

public:
    explicit Texture2DResource(FDesc desc);

    void OnCreateGpuResources() override;
    void OnDestroyGpuResources() override;

    [[nodiscard]] RTextureHandle GetTexture() const { return m_Texture; }
    [[nodiscard]] const std::string& GetSourcePath() const { return m_Desc.path; }

private:
    void LoadCPU();
    void UploadGPU();
    void ReleaseCPU();

private:
    FDesc m_Desc;

    // CPU staging
    int m_W = 0, m_H = 0;
    std::vector<unsigned char> m_Pixels;
    bool m_CpuReady = false;

    // GPU
    RTextureHandle m_Texture{};
};

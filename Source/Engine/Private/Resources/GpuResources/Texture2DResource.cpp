//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Texture2DResource.h"

#include <iostream>
#include <filesystem>

#include "Rendering/IRenderDevice.h"
#include "stb/stb_image.h"
#include "Utilities/UPathFinder.h"

namespace
{
    static std::string NormalizeSlashes(std::string s)
    {
        for (char& c : s)
            if (c == '\\') c = '/';
        return s;
    }

    // Accept:
    //  - absolute filesystem path
    //  - project-relative path (e.g. "Assets/Editor/Icons/Move.png")
    static std::string ResolveToAbsolutePath(const std::string& inPath)
    {
        namespace fs = std::filesystem;

        std::string p = NormalizeSlashes(inPath);
        fs::path fp(p);

        if (fp.is_absolute())
            return UPathFinder::Normalize(p);

        // Treat as project-relative
        const std::string projectRoot = UPathFinder::ResolvePath("");
        return UPathFinder::Normalize(UPathFinder::Join(projectRoot, p));
    }
}

Texture2DResource::Texture2DResource(FDesc desc)
    : m_Desc(std::move(desc))
{
    stbi_set_flip_vertically_on_load(m_Desc.bFlipY ? 1 : 0);
}

void Texture2DResource::OnCreateGpuResources()
{
    if (!m_CpuReady)
        LoadCPU();

    if (!m_CpuReady)
        return;

    UploadGPU();
    ReleaseCPU();
}

void Texture2DResource::OnDestroyGpuResources()
{
    IRenderDevice* dev = GetDevice();
    if (!dev)
        return;

    if (m_Texture.IsValid())
        dev->DestroyTexture(m_Texture);

    m_Texture = {};
}

void Texture2DResource::LoadCPU()
{
    m_Pixels.clear();
    m_W = 0;
    m_H = 0;

    const std::string absPath = ResolveToAbsolutePath(m_Desc.path);

    int comp = 0;
    unsigned char* data = stbi_load(absPath.c_str(), &m_W, &m_H, &comp, 4);
    if (!data)
    {
        std::cerr << "[Texture2DResource]: Failed to load image: " << absPath << "\n";
        m_CpuReady = false;
        return;
    }

    const size_t byteCount = static_cast<size_t>(m_W) * static_cast<size_t>(m_H) * 4u;
    m_Pixels.assign(data, data + byteCount);
    stbi_image_free(data);

    m_CpuReady = true;
}

void Texture2DResource::UploadGPU()
{
    IRenderDevice* dev = GetDevice();
    if (!dev)
    {
        std::cerr << "[Texture2DResource]: UploadGPU failed: device is null.\n";
        return;
    }

    if (m_W <= 0 || m_H <= 0 || m_Pixels.empty())
    {
        std::cerr << "[Texture2DResource]: UploadGPU failed: invalid CPU staging.\n";
        return;
    }

    RTexture ro{};
    ro.type = ETexType::Tex2D;
    ro.width = m_W;
    ro.height = m_H;
    ro.channels = 4;
    ro.data = m_Pixels.data();
    ro.format = m_Desc.bSRGB ? ETexFormat::SRGB8_A8 : ETexFormat::RGBA8;
    ro.bGenerateMipmaps = m_Desc.bGenerateMipmaps;

    ro.wrapS = m_Desc.wrapS;
    ro.wrapT = m_Desc.wrapT;
    ro.minFilter = m_Desc.minFilter;
    ro.magFilter = m_Desc.magFilter;

    ro.bSRGB = m_Desc.bSRGB;

    // Destroy previous GPU texture if re-creating (safety)
    if (m_Texture.IsValid())
        dev->DestroyTexture(m_Texture);

    m_Texture = dev->CreateTexture(ro);

    if (!m_Texture.IsValid())
        std::cerr << "[Texture2DResource]: CreateTexture failed for: " << m_Desc.path << "\n";
}

void Texture2DResource::ReleaseCPU()
{
    m_Pixels.clear();
    m_Pixels.shrink_to_fit(); // optional: free memory aggressively for editor startup burst
    m_W = 0;
    m_H = 0;
    m_CpuReady = false;
}
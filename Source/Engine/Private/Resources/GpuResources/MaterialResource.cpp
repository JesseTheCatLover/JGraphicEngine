//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "MaterialResource.h"

#include "Texture2DResource.h"
#include "Assets/Payloads/FMaterialPayloadHeader.h"
#include "Assets/AssetRegistrySubsystem.h"
#include "Rendering/IRenderDevice.h"
#include "Resources/ResourceSubsystem.h"

MaterialResource::MaterialResource(FDesc desc,ResourceSubsystem* resources,
                                   AssetRegistrySubsystem* registry)
    : GpuResource(registry)
    , m_Desc(std::move(desc))
    , m_Resources(resources)
    , m_Registry(registry)
{
}

bool MaterialResource::OnCreateGpuResources()
{
    if (!m_CpuReady)
        LoadCPU();

    if (!m_CpuReady)
        return false;

    // Acquire RendererSubsystem to register this material
    if (!GetDevice())
        return false;

    UploadGPU(GetDevice());
    ReleaseCPU();

    return true;
}

void MaterialResource::OnDestroyGpuResources()
{
    if (!GetDevice())
        return;

    if (m_Handle.IsValid())
    {
        GetDevice()->DestroyMaterial(m_Handle);
        m_Handle = RMaterialHandle::Invalid();
    }
}

void MaterialResource::LoadCPU()
{
    FAssetHeader header{};
    std::vector<uint8_t> payload;

    if (!LoadAssetBinary(m_Desc.assetID, header, payload))
        return;

    // --- Size Check ---
    // Check against Header + Params + all texture paths (which are variable length)
    // For now, we rely on reading the strings and ensuring we don't run past the end.
    if (payload.size() < sizeof(FMaterialPayloadHeader) + sizeof(FMaterialParams))
        return;

    const uint8_t* base = payload.data();
    size_t offset = 0;

    auto ensureAvailable = [&](uint64_t needed) -> bool
    {
        return offset + needed <= payload.size();
    };

    // -----------------------------
    // Read payload header
    // -----------------------------
    const auto* matHeader =
        reinterpret_cast<const FMaterialPayloadHeader*>(base + offset);
    offset += sizeof(FMaterialPayloadHeader);

    if (!ensureAvailable(sizeof(FMaterialParams)))
        return;

    // -----------------------------
    // Read material params
    // -----------------------------
    const auto* matParams =
        reinterpret_cast<const FMaterialParams*>(base + offset);
    offset += sizeof(FMaterialParams);

    // -----------------------------
    // Helper: read string
    // -----------------------------
    auto readString = [&](uint32_t len) -> std::string
    {
        std::string out;
        if (len > 0)
        {
            if (!ensureAvailable(len))
                return {}; // malformed payload

            out.assign(
                reinterpret_cast<const char*>(base + offset),
                reinterpret_cast<const char*>(base + offset) + len
            );
            offset += len;
        }
        return out;
    };

    // -----------------------------
    // Read ALL texture paths from payload in order
    // -----------------------------
    std::string baseColorTexPath      = readString(matHeader->baseColorTexturePathLength);
    std::string normalTexPath         = readString(matHeader->normalTexturePathLength);
    std::string metallicTexPath       = readString(matHeader->metallicTexturePathLength);
    std::string roughnessTexPath      = readString(matHeader->roughnessTexturePathLength);
    std::string metalRoughnessTexPath = readString(matHeader->metalRoughnessTexturePathLength);
    std::string occlusionTexPath      = readString(matHeader->occlusionTexturePathLength);
    std::string emissiveTexPath       = readString(matHeader->emissiveTexturePathLength);

    // ---------------------------------------
    // Resolve asset IDs -> texture handles
    // ---------------------------------------
    auto resolveTexture = [&](const std::string& id) -> RTextureHandle
    {
        if (id.empty())
            return RTextureHandle::Invalid();
        return GetTextureHandleForAssetID(id);
    };

// ---------------------------------------
    // Assign textures (with correct combined MR logic)
    // ---------------------------------------
    m_Surface.baseColor = resolveTexture(baseColorTexPath);
    m_Surface.normal    = resolveTexture(normalTexPath);
    m_Surface.occlusion = resolveTexture(occlusionTexPath);
    m_Surface.emissive  = resolveTexture(emissiveTexPath);

    // -------------------------------------------------
    // Prefer combined MR -> fallback to packed separate maps
    // -------------------------------------------------
    if (!metalRoughnessTexPath.empty())
    {
        // Combined metallic-roughness texture created by importer
        m_Surface.metallicRoughness = resolveTexture(metalRoughnessTexPath);
    }
    else if (!metallicTexPath.empty() || !roughnessTexPath.empty())
    {
        // If we reach here, importer did NOT generate a combined MR map.
        // This is technically an importer bug — but we fail gracefully:
        // Runtime cannot combine them; so metallic-only is better than nothing.
        m_Surface.metallicRoughness = resolveTexture(metallicTexPath);
    }
    else
    {
        // No texture -> rely on scalar values
        m_Surface.metallicRoughness = RTextureHandle::Invalid();
    }

    // ---------------------------------------
    // Copy scalar params
    // ---------------------------------------
    std::memcpy(
        m_Surface.params.baseColorFactor,
        matParams->baseColorFactor,
        sizeof(float) * 4
    );

    m_Surface.params.metallicFactor   = matParams->metallicFactor;
    m_Surface.params.roughnessFactor  = matParams->roughnessFactor;

    std::memcpy(m_Surface.params.emissiveFactor,
                matParams->emissiveFactor,
                sizeof(float) * 3);

    m_Surface.params.emissiveIntensity = matParams->emissiveIntensity;
    m_Surface.params.normalScale       = matParams->normalScale;
    m_Surface.params.occlusionStrength = matParams->occlusionStrength;

    // Copy uv tiling if we add it in the struct
    m_Surface.params.uvTiling[0] = matParams->uvTiling[0];
    m_Surface.params.uvTiling[1] = matParams->uvTiling[1];

    m_CpuReady = true;
}

void MaterialResource::UploadGPU(IRenderDevice* device)
{
    if (!device)
        return;

    m_Handle = GetDevice()->CreateMaterial(m_Surface);
}

void MaterialResource::ReleaseCPU()
{
    m_Surface = FSurfaceDesc{};
    m_CpuReady = false;
}

RTextureHandle MaterialResource::GetTextureHandleForAssetID(const std::string &assetID)
{
    if (assetID.empty() || !m_Resources)
        return RTextureHandle::Invalid();

    auto texRes = m_Resources->Load<Texture2DResource>(
        assetID,Texture2DResource::FDesc{ assetID });

    if (!texRes)
        return RTextureHandle::Invalid();

    return texRes->GetTexture();
}

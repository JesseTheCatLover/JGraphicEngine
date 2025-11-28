// Copyright 2025 JesseTheCatLover

#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "Core/JCoreObject.h"
#include "Resources/GpuResource.h"
#include "Rendering/RObjects.h"
#include "Rendering/RHandles.h"

class IRenderDevice;

/**
 * @class ModelResource
 * @brief Loads a 3D model (Assimp), stages CPU data, builds GPU meshes/textures/materials.
 *
 * Design:
 *  - LoadCPU(): parse model file with Assimp, bake interleaved vertices/indices, gather textures.
 *  - UploadGPU(): create RTextureHandle(s), RMaterialHandle(s), RMeshHandle(s).
 *  - ReleaseCPU(): optional step to drop CPU staging memory after upload.
 */
class ModelResource : public GpuResource // TODO: Temp
{
public:
    explicit ModelResource(std::string sourcePath);

    // GpuResource
    void OnCreateGpuResources() override;
    void OnDestroyGpuResources() override;

    struct FSubmeshGPU
    {
        RMeshHandle     mesh{};
        RMaterialHandle material{};
    };

    /** @return Immutable view of submeshes (one draw per entry). */
    const std::vector<FSubmeshGPU>& GetSubmeshes() const { return m_SubmeshesGPU; }

    /** @return Source key/path this model was created with. */
    const std::string& GetSource() const { return m_Source; }

private:
    // -------- CPU staging models --------
    struct FMeshCPU {
        std::vector<float>    vtx; // interleaved [pos(3), normal(3), uv(2), tangent(3), bitangent(3)] if present
        std::vector<uint32_t> idx;
        RMesh                 ro;      // upload descriptor (vertexStride, attributes, etc.)
        int                   materialIndex = -1;
    };

    struct FTexCPU {
        std::string                   path;     // absolute resolved path
        int                           w = 0, h = 0, channels = 4;
        std::vector<unsigned char>    pixels;   // RGBA8
        RTexture                      ro;       // upload descriptor
    };

    struct FMatCPU {
        int baseColorTex = -1; // index into m_TexturesCPU, or -1
    };

private:
    // CPU data
    std::vector<FMeshCPU> m_MeshesCPU;
    std::vector<FTexCPU>  m_TexturesCPU;   // unique textures deduped by absolute path
    std::vector<FMatCPU>  m_MaterialsCPU;
    bool                  m_CpuReady = false;

    // GPU data
    std::vector<RTextureHandle>   m_TexturesGPU;
    std::vector<RMaterialHandle>  m_MaterialsGPU;
    std::vector<FSubmeshGPU>      m_SubmeshesGPU;

    // Bookkeeping
    std::string m_Source; // project-relative key given to ResourceManager

private:
    // Pipeline steps
    void LoadCPU();     // Reads with Assimp + stb_image, fills m_*CPU
    void UploadGPU();   // Consumes CPU staging to create R*Handles
    void ReleaseCPU();  // Optional: free CPU staging after upload

    // Helpers
    int  AcquireTexture(const std::string& absPath, bool srgb);
    void FillMeshUploadDesc(FMeshCPU& outMesh, bool hasNormals, bool hasUVs, bool hasTangents);

    // Dedup map for textures (absolute path -> index in m_TexturesCPU)
    std::unordered_map<std::string, int> m_TexIndexByPath;
};

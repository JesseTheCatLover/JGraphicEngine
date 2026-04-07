//  Copyright 2025-2026 JesseTheCatLlover. All Rights Reserved.

#include "Assets/Importers/ModelImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cstring>
#include <iostream>

#include "Assets/AssetFile.h"
#include "Assets/Payloads/FStaticMeshPayloadHeader.h"
#include "Assets/Payloads/FMaterialPayloadHeader.h"
#include "Utilities/UPath.h"
#include "Utilities/UUUID.h"

namespace
{
    // ----------------------------------------------------------------------
    // Vertex Layout
    // ----------------------------------------------------------------------
    struct FVertexPointUV
    {
        float position[3];
        float normal[3];
        float tangent[3];
        float texcoord[2];
    };

    // ----------------------------------------------------------------------
    // Scene Extraction Helpers
    // ----------------------------------------------------------------------

    void ExtractMeshData(const aiMesh* mesh,
                         std::vector<FVertexPointUV>& outVerts,
                         std::vector<uint32_t>& outIndices)
    {
        const size_t baseVertCount = outVerts.size();
        outVerts.reserve(baseVertCount + mesh->mNumVertices);

        for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
        {
            FVertexPointUV v{};
            v.position[0] = mesh->mVertices[i].x;
            v.position[1] = mesh->mVertices[i].y;
            v.position[2] = mesh->mVertices[i].z;

            if (mesh->HasNormals())
            {
                v.normal[0] = mesh->mNormals[i].x;
                v.normal[1] = mesh->mNormals[i].y;
                v.normal[2] = mesh->mNormals[i].z;
            }

            if (mesh->HasTangentsAndBitangents())
            {
                v.tangent[0] = mesh->mTangents[i].x;
                v.tangent[1] = mesh->mTangents[i].y;
                v.tangent[2] = mesh->mTangents[i].z;
            }

            if (mesh->HasTextureCoords(0))
            {
                v.texcoord[0] = mesh->mTextureCoords[0][i].x;
                v.texcoord[1] = mesh->mTextureCoords[0][i].y;
            }

            outVerts.push_back(v);
        }

        const size_t baseIndexCount = outIndices.size();
        outIndices.reserve(baseIndexCount + mesh->mNumFaces * 3);

        for (uint32_t f = 0; f < mesh->mNumFaces; ++f)
        {
            const aiFace& face = mesh->mFaces[f];
            for (unsigned j = 0; j < face.mNumIndices; ++j)
            {
                outIndices.push_back(static_cast<uint32_t>(baseVertCount) + face.mIndices[j]);
            }
        }
    }

    // ----------------------------------------------------------------------
    // MATERIAL IMPORT
    // ----------------------------------------------------------------------

    struct FImportedMaterialTemp
    {
        std::string name;

        std::string baseColorTexturePath;
        std::string normalTexturePath;
        std::string metallicTexturePath;
        std::string roughnessTexturePath;
        std::string metalRoughnessTexturePath;
        std::string occlusionTexturePath;
        std::string emissiveTexturePath;

        float uvTiling[2] = { 1.0f, 1.0f }; // Default to 1:1
    };

    std::vector<FImportedMaterialTemp> CollectMaterials(const aiScene* scene)
    {
        std::vector<FImportedMaterialTemp> out;
        out.reserve(scene->mNumMaterials);

        for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
        {
            aiMaterial* mat = scene->mMaterials[i];

            // Name
            aiString aiName;
            mat->Get(AI_MATKEY_NAME, aiName);
            std::string matName = aiName.length ? aiName.C_Str() : "Material";

            FImportedMaterialTemp temp{};
            temp.name = std::move(matName);

            // Base color / diffuse texture
            aiString texPath;
            if (mat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == AI_SUCCESS ||
                mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
            {
                temp.baseColorTexturePath = texPath.C_Str();
            }

            // Normal map
            if (mat->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS)
            {
                temp.normalTexturePath = texPath.C_Str();
            }

            // Metallic (PBR)
            if (mat->GetTexture(aiTextureType_METALNESS, 0, &texPath) == AI_SUCCESS)
            {
                temp.metallicTexturePath = texPath.C_Str();
            }

            // Roughness (PBR)
            if (mat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texPath) == AI_SUCCESS)
            {
                temp.roughnessTexturePath = texPath.C_Str();
            }

            // Occlusion (often stored as "Lightmap" in Assimp for glTF)
            if (mat->GetTexture(aiTextureType_LIGHTMAP, 0, &texPath) == AI_SUCCESS)
            {
                temp.occlusionTexturePath = texPath.C_Str();
            }

            // Emissive
            if (mat->GetTexture(aiTextureType_EMISSIVE, 0, &texPath) == AI_SUCCESS)
            {
                temp.emissiveTexturePath = texPath.C_Str();
            }

            // Metallic-Roughness packed (commonly in glTF as a single combined map).
            // Many loaders encode this as aiTextureType_UNKNOWN. We treat the first UNKNOWN
            // as the packed metallic-roughness map.
            if (mat->GetTexture(aiTextureType_UNKNOWN, 0, &texPath) == AI_SUCCESS)
            {
                temp.metalRoughnessTexturePath = texPath.C_Str();
            }

            // UV tiling (scaling) from base color or diffuse slot, if present.
            // If not present, defaults remain 1:1.
            aiUVTransform uvTransform;
            if (mat->Get(AI_MATKEY_UVTRANSFORM(aiTextureType_BASE_COLOR, 0), uvTransform) == AI_SUCCESS)
            {
                temp.uvTiling[0] = uvTransform.mScaling.x;
                temp.uvTiling[1] = uvTransform.mScaling.y;
            }
            else if (mat->Get(AI_MATKEY_UVTRANSFORM(aiTextureType_DIFFUSE, 0), uvTransform) == AI_SUCCESS)
            {
                temp.uvTiling[0] = uvTransform.mScaling.x;
                temp.uvTiling[1] = uvTransform.mScaling.y;
            }

            out.push_back(std::move(temp));
        }

        return out;
    }

    bool WriteMaterialAsset(const FImportedMaterialTemp& material,
                            const std::string& baseVirtualPath,
                            const std::string& basePhysicalPath,
                            size_t materialIndex,
                            FAssetImportResult& outResult)
  {
        // Base file name (without extension) of the *destination* .jasset file root
        // e.g. "/Game/Meshes/crate.jasset" -> baseName = "crate"
        const std::string baseName = UPath::GetFileName(basePhysicalPath, false);

        // Physical path: same directory, suffixed, .jasset
        // e.g. "/Game/Meshes/crate_mat0.jasset"
        const std::string matFileName    = baseName + "_mat" + std::to_string(materialIndex) + ".jasset";
        const std::string matPhysicalDir = UPath::GetParent(basePhysicalPath);
        const std::string matPhysicalPath = UPath::Join(matPhysicalDir, matFileName);

        // Virtual path: same rule, just on the virtual side
        const std::string matVirtualName = baseName + "_mat" + std::to_string(materialIndex);
        const std::string matVirtualDir  = UPath::GetParent(baseVirtualPath);
        const std::string matVirtualPath = UPath::Join(matVirtualDir, matVirtualName);

        // Build payload
        FMaterialPayloadHeader header{};
        header.version = 1;

        // Flags / params
        header.flags = 0;
        if (!material.baseColorTexturePath.empty())
        {
            header.flags |= MATERIAL_FLAG_HAS_BASE_COLOR_TEXTURE;
        }
        if (!material.normalTexturePath.empty())
        {
            header.flags |= MATERIAL_FLAG_HAS_NORMAL_TEXTURE;
        }
        if (!material.metallicTexturePath.empty())
        {
            header.flags |= MATERIAL_FLAG_HAS_METALLIC_TEXTURE;
        }
        if (!material.roughnessTexturePath.empty())
        {
            header.flags |= MATERIAL_FLAG_HAS_ROUGHNESS_TEXTURE;
        }
        if (!material.metalRoughnessTexturePath.empty())
        {
            header.flags |= MATERIAL_FLAG_HAS_METAL_ROUGHNESS_MAP;
        }
        if (!material.occlusionTexturePath.empty())
        {
            header.flags |= MATERIAL_FLAG_HAS_OCCLUSION_TEXTURE;
        }
        if (!material.emissiveTexturePath.empty())
        {
            header.flags |= MATERIAL_FLAG_HAS_EMISSIVE_TEXTURE;
        }

        // For now we only support base color texture + default params
        // (factors may be extended later as needed).
        FMaterialParams params{};
        params.baseColorFactor[0] = 1.0f;
        params.baseColorFactor[1] = 1.0f;
        params.baseColorFactor[2] = 1.0f;
        params.baseColorFactor[3] = 1.0f;
        params.metallicFactor     = 0.0f;
        params.roughnessFactor    = 1.0f;
        params.emissiveFactor[0]  = 0.0f;
        params.emissiveFactor[1]  = 0.0f;
        params.emissiveFactor[2]  = 0.0f;
        params.emissiveIntensity  = 0.0f;

        // UV tiling
        params.uvTiling[0] = material.uvTiling[0];
        params.uvTiling[1] = material.uvTiling[1];

        // Path lengths
        header.baseColorTexturePathLength      =
            static_cast<uint32_t>(material.baseColorTexturePath.size());
        header.normalTexturePathLength         =
            static_cast<uint32_t>(material.normalTexturePath.size());
        header.metallicTexturePathLength       =
            static_cast<uint32_t>(material.metallicTexturePath.size());
        header.roughnessTexturePathLength      =
            static_cast<uint32_t>(material.roughnessTexturePath.size());
        header.metalRoughnessTexturePathLength =
            static_cast<uint32_t>(material.metalRoughnessTexturePath.size());
        header.occlusionTexturePathLength      =
            static_cast<uint32_t>(material.occlusionTexturePath.size());
        header.emissiveTexturePathLength       =
            static_cast<uint32_t>(material.emissiveTexturePath.size());

        const size_t payloadSize =
            sizeof(FMaterialPayloadHeader) +
            sizeof(FMaterialParams) +
            header.baseColorTexturePathLength +
            header.normalTexturePathLength +
            header.metallicTexturePathLength +
            header.roughnessTexturePathLength +
            header.metalRoughnessTexturePathLength +
            header.occlusionTexturePathLength +
            header.emissiveTexturePathLength;

        std::vector<uint8_t> payload(payloadSize);
        size_t offset = 0;

        auto write = [&](const void* data, size_t size)
        {
            std::memcpy(payload.data() + offset, data, size);
            offset += size;
        };

        write(&header, sizeof(header));
        write(&params, sizeof(params));

        if (!material.baseColorTexturePath.empty())
        {
            write(material.baseColorTexturePath.data(), material.baseColorTexturePath.size());
        }
        if (!material.normalTexturePath.empty())
        {
            write(material.normalTexturePath.data(), material.normalTexturePath.size());
        }
        if (!material.metallicTexturePath.empty())
        {
            write(material.metallicTexturePath.data(), material.metallicTexturePath.size());
        }
        if (!material.roughnessTexturePath.empty())
        {
            write(material.roughnessTexturePath.data(), material.roughnessTexturePath.size());
        }
        if (!material.metalRoughnessTexturePath.empty())
        {
            write(material.metalRoughnessTexturePath.data(), material.metalRoughnessTexturePath.size());
        }
        if (!material.occlusionTexturePath.empty())
        {
            write(material.occlusionTexturePath.data(), material.occlusionTexturePath.size());
        }
        if (!material.emissiveTexturePath.empty())
        {
            write(material.emissiveTexturePath.data(), material.emissiveTexturePath.size());
        }

        // Asset header
        FAssetHeader assetHeader{};
        assetHeader.assetID          = UUUID::GenerateUUID();
        assetHeader.assetName        = material.name;
        assetHeader.assetType        = EAssetType::Material;
        assetHeader.encoding         = EAssetEncoding::Binary;
        assetHeader.containerVersion = FAssetHeader::CurrentContainerVersion;
        assetHeader.payloadVersion   = header.version;
        assetHeader.sourcePath       = basePhysicalPath;
        assetHeader.importerName     = "ModelImporter";

        if (!AssetFile::WriteBinaryAsset(matPhysicalPath, assetHeader, payload))
        {
            outResult.errors.push_back("Failed to write material asset: " + matPhysicalPath);
            return false;
        }
        std::cout << "Material: " << material.name << "\n";
        std::cout << "BaseColor: " << material.baseColorTexturePath << "\n";


        FImportedAssetInfo created{};
        created.assetID      = assetHeader.assetID;
        created.assetType    = EAssetType::Material;
        created.virtualPath  = matVirtualPath;
        created.physicalPath = matPhysicalPath;

        outResult.createdAssets.push_back(std::move(created));
        return true;
    }

    // ----------------------------------------------------------------------
    // STATIC MESH IMPORT
    // ----------------------------------------------------------------------

    struct FImportedMeshTemp
    {
        std::string name;

        std::vector<FVertexPointUV> vertices;
        std::vector<uint32_t> indices;
        std::vector<FModelSubMesh> subMeshes;

        std::vector<FMaterialSlot> materialSlots;

        std::vector<std::string> materialSlotNames;
        std::vector<std::string> materialSlotAssetIDs;
    };

    static FImportedMeshTemp BuildSingleMeshFromScene(
     const aiScene* scene,
     const std::vector<FImportedMaterialTemp>& materials)
    {
        FImportedMeshTemp result{};
        result.name = "Mesh";

        std::vector<FVertexPointUV>& vertices = result.vertices;
        std::vector<uint32_t>& indices        = result.indices;
        std::vector<FModelSubMesh>& subMeshes = result.subMeshes;

        vertices.clear();
        indices.clear();
        subMeshes.clear();

        // Build submeshes
        for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
        {
            const aiMesh* mesh = scene->mMeshes[i];

            const uint32_t firstIndex = static_cast<uint32_t>(indices.size());

            ExtractMeshData(mesh, vertices, indices);

            FModelSubMesh sm{};
            sm.firstIndex    = firstIndex;
            sm.indexCount    = mesh->mNumFaces * 3;
            sm.materialIndex = mesh->mMaterialIndex;

            subMeshes.push_back(sm);
        }

        // Material slots
        const size_t materialCount = materials.size();

        result.materialSlots.resize(materialCount);
        result.materialSlotNames.resize(materialCount);
        result.materialSlotAssetIDs.resize(materialCount);

        for (size_t i = 0; i < materialCount; ++i)
        {
            const auto& srcMat = materials[i];
            auto& slot = result.materialSlots[i];

            result.materialSlotNames[i] = srcMat.name;
            result.materialSlotAssetIDs[i] = "";

            slot.nameLength = (uint32_t)result.materialSlotNames[i].size();
            slot.materialAssetIDLength = 0;
        }

        return result;
    }

    static bool WriteStaticMeshAsset(const FImportedMeshTemp& meshTemp,
                                     const std::string& baseVirtualPath,
                                     const std::string& basePhysicalPath,
                                     FAssetImportResult& outResult)
    {
        const std::string baseName = UPath::GetFileName(basePhysicalPath, false);

        // e.g. crate.jasset (static mesh)
        const std::string meshFileName     = baseName + ".jasset";
        const std::string meshPhysicalDir  = UPath::GetParent(basePhysicalPath);
        const std::string meshPhysicalPath = UPath::Join(meshPhysicalDir, meshFileName);

        const std::string meshVirtualName = baseName;
        const std::string meshVirtualDir  = UPath::GetParent(baseVirtualPath);
        const std::string meshVirtualPath = UPath::Join(meshVirtualDir, meshVirtualName);

        const auto& vertices      = meshTemp.vertices;
        const auto& indices       = meshTemp.indices;
        const auto& subMeshes     = meshTemp.subMeshes;
        const auto& materialSlots = meshTemp.materialSlots;

        FStaticMeshPayloadHeader header{};
        header.version = 1;

        header.vertexCount      = static_cast<uint32_t>(vertices.size());
        header.indexCount       = static_cast<uint32_t>(indices.size());
        header.subMeshCount     = static_cast<uint32_t>(subMeshes.size());
        header.materialSlotCount = static_cast<uint32_t>(materialSlots.size()); // *** FIXED

        header.vertexStride  = sizeof(FVertexPointUV);
        header.bHasNormals   = 1;
        header.bHasTangents  = 1;
        header.bHasUVs       = 1;
        header.reserved      = 0;

        header.vertexBufferSize  =
            static_cast<uint64_t>(vertices.size()) * sizeof(FVertexPointUV);
        header.indexBufferSize   =
            static_cast<uint64_t>(indices.size()) * sizeof(uint32_t);
        header.subMeshTableSize  =
            static_cast<uint64_t>(subMeshes.size()) * sizeof(FModelSubMesh);

        // Compute material slot table + string blob sizes
        uint64_t materialSlotsTableSize = 0;
        uint64_t materialStringsSize    = 0;

        materialSlotsTableSize = static_cast<uint64_t>(materialSlots.size()) * sizeof(FMaterialSlot);

        for (size_t i = 0; i < materialSlots.size(); ++i)
        {
            materialStringsSize += meshTemp.materialSlotNames[i].size();
            materialStringsSize += meshTemp.materialSlotAssetIDs[i].size();
        }

        // Total payload:
        // header
        // vertex buffer
        // index buffer
        // submesh table
        // material slot table
        // material slot strings blob (names + material asset IDs)
        const uint64_t totalSize =
            sizeof(FStaticMeshPayloadHeader) +
            header.vertexBufferSize +
            header.indexBufferSize +
            header.subMeshTableSize +
            materialSlotsTableSize +
            materialStringsSize;

        std::vector<uint8_t> payload(static_cast<size_t>(totalSize));
        size_t offset = 0;

        auto write = [&](const void* data, size_t size)
        {
            std::memcpy(payload.data() + offset, data, size);
            offset += size;
        };

        // header
        write(&header, sizeof(header));

        // vertex buffer
        if (!vertices.empty())
        {
            write(vertices.data(), static_cast<size_t>(header.vertexBufferSize));
        }

        // index buffer
        if (!indices.empty())
        {
            write(indices.data(), static_cast<size_t>(header.indexBufferSize));
        }

        // submesh table
        if (!subMeshes.empty())
        {
            write(subMeshes.data(), static_cast<size_t>(header.subMeshTableSize));
        }

        // material slot table
        if (!materialSlots.empty())
        {
            write(materialSlots.data(), static_cast<size_t>(materialSlotsTableSize));
        }

        // material slot strings blob
        for (size_t i = 0; i < materialSlots.size(); ++i)
        {
            const std::string& nameStr = meshTemp.materialSlotNames[i];
            const std::string& idStr = meshTemp.materialSlotAssetIDs[i];

            if (!nameStr.empty())
            {
                write(nameStr.data(), nameStr.size());
            }

            if (!idStr.empty())
            {
                write(idStr.data(), idStr.size());
            }
        }

        // Asset header
        FAssetHeader assetHeader{};
        assetHeader.assetID          = UUUID::GenerateUUID();
        assetHeader.assetName        = meshVirtualName;
        assetHeader.assetType        = EAssetType::StaticMesh;
        assetHeader.encoding         = EAssetEncoding::Binary;
        assetHeader.containerVersion = FAssetHeader::CurrentContainerVersion;
        assetHeader.payloadVersion   = header.version;
        assetHeader.sourcePath       = basePhysicalPath;
        assetHeader.importerName     = "ModelImporter";

        if (!AssetFile::WriteBinaryAsset(meshPhysicalPath, assetHeader, payload))
        {
            outResult.errors.push_back("Failed to write static mesh asset: " + meshPhysicalPath);
            return false;
        }

        FImportedAssetInfo created{};
        created.assetID      = assetHeader.assetID;
        created.assetType    = EAssetType::StaticMesh;
        created.virtualPath  = meshVirtualPath;
        created.physicalPath = meshPhysicalPath;

        outResult.createdAssets.push_back(std::move(created));
        return true;
    }
} // anonymous namespace

// ----------------------------------------------------------------------
// ModelImporter Implementation
// ----------------------------------------------------------------------

bool ModelImporter::OnImport(const FAssetImportRequest& request,
                             const VirtualPathMounter& pathMounter,
                             const std::string& destinationVirtualPath,
                             const std::string& destinationPhysicalPath,
                             FAssetImportResult& outResult) const
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        request.sourceFilePath,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_SortByPType |
        aiProcess_FlipUVs
    );

    if (!scene || !scene->HasMeshes())
    {
        outResult.errors.push_back("Assimp failed to load: " + std::string(importer.GetErrorString()));
        outResult.bSuccess = false;
        return false;
    }

    const std::string normalizedSourcePath = UPath::Normalize(request.sourceFilePath);
    (void)normalizedSourcePath; // currently unused

    // 1) Import materials (one .jasset per aiMaterial)
    const auto materials = CollectMaterials(scene);

    std::vector<std::string> materialAssetIDs;
    materialAssetIDs.reserve(materials.size());

    for (size_t i = 0; i < materials.size(); ++i)
    {
        if (!WriteMaterialAsset(materials[i], destinationVirtualPath, destinationPhysicalPath, i,outResult))
        {
            outResult.bSuccess = false;
            return false;
        }

        materialAssetIDs.push_back(outResult.createdAssets.back().assetID);
    }

    // 2) Import static mesh (single combined mesh with submeshes)
    FImportedMeshTemp meshTemp = BuildSingleMeshFromScene(scene, materials);
    for (size_t i = 0; i < meshTemp.materialSlots.size(); ++i)
    {
        meshTemp.materialSlotAssetIDs[i] = materialAssetIDs[i];
        meshTemp.materialSlots[i].materialAssetIDLength = (uint32_t)materialAssetIDs[i].size();
    }

    // Write mesh asset
    if (!WriteStaticMeshAsset(meshTemp, destinationVirtualPath, destinationPhysicalPath, outResult))
    {
        outResult.bSuccess = false;
        return false;
    }

    // (Future) skeletal mesh, skeleton, animations...

    outResult.bSuccess = true;
    return true;
}

std::vector<std::string> ModelImporter::GetSupportedSourceExtensions() const
{
    return { "fbx", "gltf", "glb", "obj" };
}

//  Copyright 2025-2026 JesseTheCatLlover. All Rights Reserved.

#include "Assets/Importers/ModelImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cstring>

#include "Assets/AssetFile.h"
#include "Assets/AssetImportSubsystem.h"
#include "Assets/Payloads/FStaticMeshPayloadHeader.h"
#include "Assets/Payloads/FMaterialPayloadHeader.h"
#include "Core/JEngine.h"
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
        float texcoord[2];
        float tangent[3];
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

            if (mesh->HasTextureCoords(0))
            {
                v.texcoord[0] = mesh->mTextureCoords[0][i].x;
                v.texcoord[1] = mesh->mTextureCoords[0][i].y;
            }
            else
            {
                v.texcoord[0] = 0.0f;
                v.texcoord[1] = 0.0f;
            }

            if (mesh->HasTangentsAndBitangents())
            {
                v.tangent[0] = mesh->mTangents[i].x;
                v.tangent[1] = mesh->mTangents[i].y;
                v.tangent[2] = mesh->mTangents[i].z;
            }
            else
            {
                v.tangent[0] = 0.0f;
                v.tangent[1] = 0.0f;
                v.tangent[2] = 0.0f;
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

        // Source file paths from Assimp
        std::string baseColorTexturePath;
        std::string normalTexturePath;
        std::string metallicTexturePath;
        std::string roughnessTexturePath;
        std::string metalRoughnessTexturePath;
        std::string occlusionTexturePath;
        std::string emissiveTexturePath;

        // Resulting engine asset IDs (after texture import)
        std::string baseColorTextureAssetID;
        std::string normalTextureAssetID;
        std::string metallicTextureAssetID;
        std::string roughnessTextureAssetID;
        std::string metalRoughnessTextureAssetID;
        std::string occlusionTextureAssetID;
        std::string emissiveTextureAssetID;

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
                            const std::string& assetBaseName,
                            const std::string& destinationVirtualFolder,
                            const std::string& destinationPhysicalFolder,
                            size_t materialIndex,
                            FAssetImportResult& outResult)
    {
        const std::string matFileName =
        assetBaseName + "_mat" + std::to_string(materialIndex) + ".jasset";

        const std::string matPhysicalPath =
            UPath::Join(destinationPhysicalFolder, matFileName);

        const std::string matVirtualName =
            assetBaseName + "_mat" + std::to_string(materialIndex);

        const std::string matVirtualPath =
            UPath::Join(destinationVirtualFolder, matVirtualName);

        // Build payload
        FMaterialPayloadHeader header{};
        header.version = 1;

        // Flags / params
        header.flags = 0;
        if (!material.baseColorTextureAssetID.empty())
        {
            header.flags |= MATERIAL_FLAG_HAS_BASE_COLOR_TEXTURE;
        }
        if (!material.normalTextureAssetID.empty())
        {
            header.flags |= MATERIAL_FLAG_HAS_NORMAL_TEXTURE;
        }
        if (!material.metallicTextureAssetID.empty())
        {
            header.flags |= MATERIAL_FLAG_HAS_METALLIC_TEXTURE;
        }
        if (!material.roughnessTextureAssetID.empty())
        {
            header.flags |= MATERIAL_FLAG_HAS_ROUGHNESS_TEXTURE;
        }
        if (!material.metalRoughnessTextureAssetID.empty())
        {
            header.flags |= MATERIAL_FLAG_HAS_METAL_ROUGHNESS_MAP;
        }
        if (!material.occlusionTextureAssetID.empty())
        {
            header.flags |= MATERIAL_FLAG_HAS_OCCLUSION_TEXTURE;
        }
        if (!material.emissiveTextureAssetID.empty())
        {
            header.flags |= MATERIAL_FLAG_HAS_EMISSIVE_TEXTURE;
        }

        // Default param values; expand as needed
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

        header.baseColorTextureAssetIDLength = static_cast<uint32_t>(material.baseColorTextureAssetID.size());
        header.normalTextureAssetIDLength    = static_cast<uint32_t>(material.normalTextureAssetID.size());
        header.metallicTextureAssetIDLength  = static_cast<uint32_t>(material.metallicTextureAssetID.size());
        header.roughnessTextureAssetIDLength = static_cast<uint32_t>(material.roughnessTextureAssetID.size());
        header.metalRoughnessTextureAssetIDLength = static_cast<uint32_t>(material.metalRoughnessTextureAssetID.size());
        header.occlusionTextureAssetIDLength = static_cast<uint32_t>(material.occlusionTextureAssetID.size());
        header.emissiveTextureAssetIDLength  = static_cast<uint32_t>(material.emissiveTextureAssetID.size());

        const size_t payloadSize = sizeof(FMaterialPayloadHeader) + sizeof(FMaterialParams) +
            header.baseColorTextureAssetIDLength +
            header.normalTextureAssetIDLength +
            header.metallicTextureAssetIDLength +
            header.roughnessTextureAssetIDLength +
            header.metalRoughnessTextureAssetIDLength +
            header.occlusionTextureAssetIDLength +
            header.emissiveTextureAssetIDLength;

        std::vector<uint8_t> payload(payloadSize);
        size_t offset = 0;

        auto write = [&](const void* data, size_t size)
        {
            std::memcpy(payload.data() + offset, data, size);
            offset += size;
        };

        write(&header, sizeof(header));
        write(&params, sizeof(params));

        if (!material.baseColorTextureAssetID.empty())
        {
            write(material.baseColorTextureAssetID.data(), material.baseColorTextureAssetID.size());
        }
        if (!material.normalTextureAssetID.empty())
        {
            write(material.normalTextureAssetID.data(), material.normalTextureAssetID.size());
        }
        if (!material.metallicTextureAssetID.empty())
        {
            write(material.metallicTextureAssetID.data(), material.metallicTextureAssetID.size());
        }
        if (!material.roughnessTextureAssetID.empty())
        {
            write(material.roughnessTextureAssetID.data(), material.roughnessTextureAssetID.size());
        }
        if (!material.metalRoughnessTextureAssetID.empty())
        {
            write(material.metalRoughnessTextureAssetID.data(), material.metalRoughnessTextureAssetID.size());
        }
        if (!material.occlusionTextureAssetID.empty())
        {
            write(material.occlusionTextureAssetID.data(), material.occlusionTextureAssetID.size());
        }
        if (!material.emissiveTextureAssetID.empty())
        {
            write(material.emissiveTextureAssetID.data(), material.emissiveTextureAssetID.size());
        }

        // Asset header
        FAssetHeader assetHeader{};
        assetHeader.assetID          = UUUID::GenerateUUID();
        assetHeader.assetName        = material.name;
        assetHeader.assetType        = EAssetType::Material;
        assetHeader.encoding         = EAssetEncoding::Binary;
        assetHeader.containerVersion = FAssetHeader::CurrentContainerVersion;
        assetHeader.payloadVersion   = header.version;
        assetHeader.sourcePath       = matPhysicalPath;
        assetHeader.importerName     = "ModelImporter";

        if (!AssetFile::WriteBinaryAsset(matPhysicalPath, assetHeader, payload))
        {
            outResult.errors.push_back("Failed to write material asset: " + matPhysicalPath);
            return false;
        }

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

        // maps slot index -> original aiMaterial index
        std::vector<uint32_t> rawMaterialIndexForSlot;
    };

        static FImportedMeshTemp BuildSingleMeshFromScene(
        const aiScene* scene,
        const std::vector<FImportedMaterialTemp>& materials)
    {
        FImportedMeshTemp result{};
        result.name = "Mesh";

        auto& vertices      = result.vertices;
        auto& indices       = result.indices;
        auto& subMeshes     = result.subMeshes;

        vertices.clear();
        indices.clear();
        subMeshes.clear();

        // ----------------------------------------------------------
        // 1. Extract meshes
        // ----------------------------------------------------------
        for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
        {
            const aiMesh* mesh = scene->mMeshes[i];

            uint32_t firstIndex = static_cast<uint32_t>(indices.size());
            ExtractMeshData(mesh, vertices, indices);

            FModelSubMesh sm{};
            sm.firstIndex = firstIndex;
            sm.indexCount = mesh->mNumFaces * 3;
            sm.materialIndex = mesh->mMaterialIndex;
            subMeshes.push_back(sm);
        }

        // ----------------------------------------------------------
        // 2. Gather *used* material indices
        // ----------------------------------------------------------
        std::vector<uint32_t> used;
        used.reserve(scene->mNumMeshes);

        for (auto& sm : subMeshes)
            used.push_back(sm.materialIndex);

        std::sort(used.begin(), used.end());
        used.erase(std::unique(used.begin(), used.end()), used.end());

        // Example: raw indices = (3,0,3,5) -> used = (0,3,5)

        // ----------------------------------------------------------
        // 3. Build remap table
        // ----------------------------------------------------------
        // rawMatIndex -> compact index
        std::unordered_map<uint32_t, uint32_t> remap;
        remap.reserve(used.size());

        for (uint32_t i = 0; i < used.size(); ++i)
            remap[used[i]] = i;

        // ----------------------------------------------------------
        // 4. Apply remap to submeshes
        // ----------------------------------------------------------
        for (auto& sm : subMeshes)
            sm.materialIndex = remap[sm.materialIndex];

        // ----------------------------------------------------------
        // 5. Build material slot arrays
        // ----------------------------------------------------------
        result.materialSlotNames.resize(used.size());
        result.materialSlotAssetIDs.resize(used.size());
        result.materialSlots.resize(used.size());
        result.rawMaterialIndexForSlot = used;

        for (size_t i = 0; i < used.size(); ++i)
        {
            uint32_t rawIndex = used[i];
            const auto& srcMat = materials[rawIndex];

            result.materialSlotNames[i]    = srcMat.name;
            result.materialSlotAssetIDs[i] = "";   // filled later in OnImport()

            result.materialSlots[i].nameLength = (uint32_t)result.materialSlotNames[i].size();
            result.materialSlots[i].materialAssetIDLength = 0;
        }

        return result;
    }

       static bool WriteStaticMeshAsset(const FImportedMeshTemp& meshTemp,
                                     const std::string& assetBaseName,
                                     const std::string& destinationVirtualFolder,
                                     const std::string& destinationPhysicalFolder,
                                     FAssetImportResult& outResult)
    {
        const std::string meshFileName = assetBaseName + ".jasset";
        const std::string meshPhysicalPath = UPath::Join(destinationPhysicalFolder, meshFileName);
        const std::string meshVirtualPath = UPath::Join(destinationVirtualFolder, assetBaseName);

        const auto& vertices = meshTemp.vertices;
        const auto& indices = meshTemp.indices;
        const auto& subMeshes = meshTemp.subMeshes;
        const auto& materialSlots = meshTemp.materialSlots;

        // ----------------------------------------------------------
        // Build header
        // ----------------------------------------------------------
        FStaticMeshPayloadHeader header{};
        header.version           = 1;
        header.vertexCount       = (uint32_t)vertices.size();
        header.indexCount        = (uint32_t)indices.size();
        header.subMeshCount      = (uint32_t)subMeshes.size();
        header.materialSlotCount = (uint32_t)materialSlots.size();

        header.vertexStride = sizeof(FVertexPointUV);
        header.bHasNormals  = 1;
        header.bHasTangents = 1;
        header.bHasUVs      = 1;
        header.reserved     = 0;

        header.vertexBufferSize = (uint64_t)vertices.size() * sizeof(FVertexPointUV);
        header.indexBufferSize  = (uint64_t)indices.size()  * sizeof(uint32_t);
        header.subMeshTableSize = (uint64_t)subMeshes.size() * sizeof(FModelSubMesh);

        // Calculate material slot string blob size
        uint64_t materialStringsSize = 0;

        for (size_t i = 0; i < materialSlots.size(); ++i)
        {
            materialStringsSize += meshTemp.materialSlotNames[i].size();
            materialStringsSize += meshTemp.materialSlotAssetIDs[i].size();
        }

        uint64_t materialSlotTableSize =
            (uint64_t)materialSlots.size() * sizeof(FMaterialSlot);

        uint64_t totalSize =
            sizeof(FStaticMeshPayloadHeader) +
            header.vertexBufferSize +
            header.indexBufferSize +
            header.subMeshTableSize +
            materialSlotTableSize +
            materialStringsSize;

        // ----------------------------------------------------------
        // Write payload
        // ----------------------------------------------------------
        std::vector<uint8_t> payload(totalSize);
        size_t offset = 0;

        auto write = [&](const void* data, size_t size)
        {
            std::memcpy(payload.data() + offset, data, size);
            offset += size;
        };

        write(&header, sizeof(header));

        if (!vertices.empty())
            write(vertices.data(), (size_t)header.vertexBufferSize);

        if (!indices.empty())
            write(indices.data(), (size_t)header.indexBufferSize);

        if (!subMeshes.empty())
            write(subMeshes.data(), (size_t)header.subMeshTableSize);

            for (size_t i = 0; i < materialSlots.size(); ++i)
            {
                // Write header
                write(&materialSlots[i], sizeof(FMaterialSlot));

                // Write name
                const auto& name = meshTemp.materialSlotNames[i];
                if (!name.empty()) write(name.data(), name.size());

                // Write asset ID
                const auto& id = meshTemp.materialSlotAssetIDs[i];
                if (!id.empty()) write(id.data(), id.size());
            }

        // ----------------------------------------------------------
        // Write asset file
        // ----------------------------------------------------------
        FAssetHeader assetHeader{};
        assetHeader.assetID          = UUUID::GenerateUUID();
        assetHeader.assetName        = assetBaseName;
        assetHeader.assetType        = EAssetType::StaticMesh;
        assetHeader.encoding         = EAssetEncoding::Binary;
        assetHeader.containerVersion = FAssetHeader::CurrentContainerVersion;
        assetHeader.payloadVersion   = header.version;
        assetHeader.sourcePath       = meshPhysicalPath;
        assetHeader.importerName     = "ModelImporter";

        if (!AssetFile::WriteBinaryAsset(meshPhysicalPath, assetHeader, payload))
        {
            outResult.errors.push_back("Failed to write static mesh: " + meshPhysicalPath);
            return false;
        }

        FImportedAssetInfo info{};
        info.assetID      = assetHeader.assetID;
        info.assetType    = EAssetType::StaticMesh;
        info.virtualPath  = meshVirtualPath;
        info.physicalPath = meshPhysicalPath;

        outResult.createdAssets.push_back(std::move(info));
        return true;
    }

} // anonymous namespace

// ----------------------------------------------------------------------
// ModelImporter Implementation
// ----------------------------------------------------------------------

std::string ModelImporter::ImportTextureIfNeeded(const std::string &texturePath, const std::string &modelSourceDir,
    const std::string &destinationVirtualFolder, AssetImportSubsystem *importer, const VirtualPathMounter &pathMounter,
    FAssetImportResult &outResult) const

{
    if (texturePath.empty())
        return "";

    // Resolve to an absolute path
    const std::string absolutePath = UPath::Normalize(UPath::Join(modelSourceDir, texturePath));

    // Check cache first
    auto it = m_TextureCache.find(absolutePath);
    if (it != m_TextureCache.end())
        return it->second;

    FAssetImportRequest texReq{};
    texReq.sourceFilePath = absolutePath;
    texReq.destinationVirtualFolder = UPath::Join(destinationVirtualFolder, "Textures");

    // Record how many assets existed before importing this texture
    const size_t beforeCount = outResult.createdAssets.size();

    if (!importer->Import(texReq, pathMounter, outResult))
        return "";

    if (outResult.createdAssets.size() <= beforeCount)
        return "";

    // Assume the last created asset from this request is the texture
    const std::string assetID = outResult.createdAssets.back().assetID;

    // Store in cache
    m_TextureCache[absolutePath] = assetID;

    return assetID;
}

bool ModelImporter::OnImport(const FAssetImportRequest& request,
                             const VirtualPathMounter& pathMounter,
                             const std::string&
                             destinationVirtualPath,
                             const std::string& destinationPhysicalPath,
                             FAssetImportResult& outResult) const
{
    Assimp::Importer assimpImporter;

    const aiScene* scene = assimpImporter.ReadFile(
        request.sourceFilePath,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_SortByPType
    );

    if (!scene || !scene->HasMeshes())
    {
        outResult.errors.push_back("Assimp failed to load: " + std::string(assimpImporter.GetErrorString()));
        outResult.bSuccess = false;
        return false;
    }

    const std::string normalizedSourcePath = UPath::Normalize(request.sourceFilePath);
    const std::string modelSourceDir = UPath::GetParent(normalizedSourcePath);
    const std::string assetBaseName = UPath::GetFileName(normalizedSourcePath, false);

    const std::string destinationPhysicalFolder = UPath::GetParent(destinationPhysicalPath);
    const std::string destinationVirtualFolder = UPath::GetParent(destinationVirtualPath);

    // 0) Get access to AssetImportSubsystem
    AssetImportSubsystem* importSubsystem = JEngine::Get().GetAssetImportSubsystem();
    if (!importSubsystem)
    {
        outResult.errors.push_back("ModelImporter: AssetImportSubsystem is null.");
        outResult.bSuccess = false;
        return false;
    }

    // 1) Collect materials from scene
    std::vector<FImportedMaterialTemp> materials = CollectMaterials(scene);

    // 1.1) Import textures for each material and fill AssetIDs
    for (auto& mat : materials)
    {
        mat.baseColorTextureAssetID =
            ImportTextureIfNeeded(mat.baseColorTexturePath, modelSourceDir, destinationVirtualFolder, importSubsystem, pathMounter, outResult);

        mat.normalTextureAssetID =
            ImportTextureIfNeeded(mat.normalTexturePath, modelSourceDir, destinationVirtualFolder, importSubsystem, pathMounter, outResult);

        mat.metallicTextureAssetID =
            ImportTextureIfNeeded(mat.metallicTexturePath, modelSourceDir, destinationVirtualFolder, importSubsystem, pathMounter, outResult);

        mat.roughnessTextureAssetID =
            ImportTextureIfNeeded(mat.roughnessTexturePath, modelSourceDir, destinationVirtualFolder, importSubsystem, pathMounter, outResult);

        mat.metalRoughnessTextureAssetID =
            ImportTextureIfNeeded(mat.metalRoughnessTexturePath, modelSourceDir, destinationVirtualFolder, importSubsystem, pathMounter, outResult);

        mat.occlusionTextureAssetID =
            ImportTextureIfNeeded(mat.occlusionTexturePath, modelSourceDir, destinationVirtualFolder, importSubsystem, pathMounter, outResult);

        mat.emissiveTextureAssetID =
            ImportTextureIfNeeded(mat.emissiveTexturePath, modelSourceDir, destinationVirtualFolder, importSubsystem, pathMounter, outResult);
    }

    // 1.2) Write material assets (one .jasset per aiMaterial)
    std::vector<std::string> materialAssetIDs;
    materialAssetIDs.reserve(materials.size());

    for (size_t i = 0; i < materials.size(); ++i)
    {
        if (!WriteMaterialAsset(materials[i], assetBaseName, destinationVirtualFolder, destinationPhysicalFolder,
                   i, outResult))
        {
            outResult.bSuccess = false;
            return false;
        }

        materialAssetIDs.push_back(outResult.createdAssets.back().assetID);
    }

    // 2) Import static mesh (single combined mesh with submeshes)
    FImportedMeshTemp meshTemp = BuildSingleMeshFromScene(scene, materials);

    // Fill material slot asset IDs on the mesh
    for (size_t slot = 0; slot < meshTemp.materialSlots.size(); ++slot)
    {
        uint32_t rawIndex = meshTemp.rawMaterialIndexForSlot[slot];

        if (rawIndex >= materialAssetIDs.size())
            continue;

        const std::string& assetID = materialAssetIDs[rawIndex];

        meshTemp.materialSlotAssetIDs[slot] = assetID;
        meshTemp.materialSlots[slot].materialAssetIDLength =
            static_cast<uint32_t>(assetID.size());
    }

    // 2.1) Write mesh asset
    if (!WriteStaticMeshAsset(meshTemp, assetBaseName, destinationVirtualFolder,
                     destinationPhysicalFolder, outResult))

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
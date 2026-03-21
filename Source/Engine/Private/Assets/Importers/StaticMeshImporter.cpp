//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Assets/Importers/StaticMeshImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Assets/AssetFile.h"
#include "Assets/Payloads/FStaticMeshPayloadHeader.h"
#include "Utilities/UPath.h"
#include "Utilities/UUUID.h"

namespace
{
    struct FVertexPointUV
    {
        float position[3];
        float normal[3];
        float tangent[3];
        float texcoord[2];
    };

    static void ExtractMeshData(const aiMesh *mesh,
                                std::vector<FVertexPointUV> &outVerts,
                                std::vector<uint32_t> &outIndices)
    {
        outVerts.reserve(mesh->mNumVertices);
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

        for (uint32_t f = 0; f < mesh->mNumFaces; ++f)
        {
            const aiFace &face = mesh->mFaces[f];
            for (unsigned j = 0; j < face.mNumIndices; ++j)
            {
                outIndices.push_back(face.mIndices[j]);
            }
        }
    }

    static std::vector<uint8_t> BuildModelPayload(const aiScene *scene)
    {
        std::vector<FVertexPointUV> vertices;
        std::vector<uint32_t> indices;
        std::vector<FModelSubMesh> subMeshes;

        std::vector<FMaterialSlot> materialSlots;
        std::vector<std::string> materialNames;
        std::vector<std::string> materialTextures;

        // ---------- Materials ----------
        materialSlots.reserve(scene->mNumMaterials);

        for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
        {
            aiMaterial *mat = scene->mMaterials[i];

            aiString name;
            mat->Get(AI_MATKEY_NAME, name);

            std::string matName = name.length ? name.C_Str() : "Material";

            aiString texPath;
            std::string tex;

            if (mat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == AI_SUCCESS ||
                mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
            {
                tex = texPath.C_Str();
            }

            FMaterialSlot slot{};
            slot.nameLength = (uint32_t) matName.size();
            slot.baseColorTextureLength = (uint32_t) tex.size();

            materialSlots.push_back(slot);
            materialNames.push_back(matName);
            materialTextures.push_back(tex);
        }

        // ---------- Meshes ----------
        for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
        {
            const aiMesh *mesh = scene->mMeshes[i];

            const uint32_t firstIndex = (uint32_t) indices.size();

            ExtractMeshData(mesh, vertices, indices);

            FModelSubMesh sm{};
            sm.firstIndex = firstIndex;
            sm.indexCount = mesh->mNumFaces * 3;
            sm.materialIndex = mesh->mMaterialIndex;

            subMeshes.push_back(sm);
        }

        // ---------- Header ----------
        FStaticMeshPayloadHeader header{};
        header.version = 1;

        header.vertexCount = (uint32_t) vertices.size();
        header.indexCount = (uint32_t) indices.size();
        header.subMeshCount = (uint32_t) subMeshes.size();
        header.materialSlotCount = (uint32_t) materialSlots.size();

        header.vertexStride = sizeof(FVertexPointUV);

        header.bHasNormals = 1;
        header.bHasTangents = 1;
        header.bHasUVs = 1;

        header.vertexBufferSize = vertices.size() * sizeof(FVertexPointUV);
        header.indexBufferSize = indices.size() * sizeof(uint32_t);
        header.subMeshTableSize = subMeshes.size() * sizeof(FModelSubMesh);

        // ---------- String Data Size ----------
        size_t stringBytes = 0;

        for (size_t i = 0; i < materialNames.size(); ++i)
        {
            stringBytes += materialNames[i].size();
            stringBytes += materialTextures[i].size();
        }

        const size_t materialTableSize = materialSlots.size() * sizeof(FMaterialSlot);

        const size_t totalSize =
                sizeof(FStaticMeshPayloadHeader) +
                header.vertexBufferSize +
                header.indexBufferSize +
                header.subMeshTableSize +
                materialTableSize +
                stringBytes;

        std::vector<uint8_t> payload(totalSize);
        size_t offset = 0;

        auto write = [&](const void *data, size_t size) {
            std::memcpy(payload.data() + offset, data, size);
            offset += size;
        };

        // ---------- Write Data ----------
        write(&header, sizeof(header));

        write(vertices.data(), header.vertexBufferSize);
        write(indices.data(), header.indexBufferSize);
        write(subMeshes.data(), header.subMeshTableSize);

        write(materialSlots.data(), materialTableSize);

        for (size_t i = 0; i < materialNames.size(); ++i)
        {
            write(materialNames[i].data(), materialNames[i].size());

            if (!materialTextures[i].empty())
            {
                write(materialTextures[i].data(), materialTextures[i].size());
            }
        }

        return payload;
    }
}

bool StaticMeshImporter::OnImport(const FAssetImportRequest& request, const VirtualPathMounter& pathMounter,
    const std::string& destinationVirtualPath, const std::string& destinationPhysicalPath,
    FAssetImportResult& outResult) const
{
    // --- Import via Assimp ---
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
        return false;
    }

    // --- Build binary payload ---
    std::vector<uint8_t> payload = BuildModelPayload(scene);

    // --- Asset header ---
    FAssetHeader header{};
    header.assetID = UUUID::GenerateUUID();
    header.assetName = UPath::GetFileName(request.sourceFilePath, false);
    header.assetType = EAssetType::StaticMesh;
    header.encoding = EAssetEncoding::Binary;
    header.containerVersion = FAssetHeader::CurrentContainerVersion;
    header.payloadVersion = 1;
    header.sourcePath = UPath::Normalize(request.sourceFilePath);
    header.importerName = GetImporterName();

    // --- Write asset ---
    if (!AssetFile::WriteBinaryAsset(destinationPhysicalPath, header, payload))
    {
        outResult.errors.push_back("Failed to write static mesh asset: " + destinationPhysicalPath);
        return false;
    }

    // --- Fill result ---
    FImportedAssetInfo created{};
    created.assetID = header.assetID;
    created.assetType = EAssetType::StaticMesh;
    created.virtualPath = destinationVirtualPath;
    created.physicalPath = destinationPhysicalPath;

    outResult.createdAssets.push_back(std::move(created));
    outResult.bSuccess = true;

    return true;
}

std::vector<std::string> StaticMeshImporter::GetSupportedSourceExtensions() const
{
    return { "fbx", "gltf", "glb", "obj" };

}

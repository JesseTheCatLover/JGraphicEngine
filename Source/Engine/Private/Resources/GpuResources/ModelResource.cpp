// Copyright 2025 JesseTheCatLover

#include "Resources/GpuResources/ModelResource.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <algorithm>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "Utilities/UPathFinder.h"
#include "Rendering/IRenderDevice.h"
#include "Rendering/FSurfaceDesc.h"

// ====== construction ======

ModelResource::ModelResource(std::string sourcePath)
    : m_Source(std::move(sourcePath))
{
    stbi_set_flip_vertically_on_load(true);
}

// ====== GpuResource hooks ======

void ModelResource::OnCreateGpuResources()
{
    if (!m_CpuReady)
        LoadCPU();

    UploadGPU();
    ReleaseCPU(); // keep RAM usage low; comment out if you want CPU copy in editor.
}

void ModelResource::OnDestroyGpuResources()
{
    IRenderDevice* dev = GetDevice();
    if (!dev) return;

    // Destroy in safe order: materials -> meshes -> textures
    for (auto& mh : m_MaterialsGPU)    if (mh.IsValid()) dev->DestroyMaterial(mh);
    for (auto& sm : m_SubmeshesGPU)    if (sm.mesh.IsValid()) dev->DestroyMesh(sm.mesh);
    for (auto& th : m_TexturesGPU)     if (th.IsValid()) dev->DestroyTexture(th);

    m_MaterialsGPU.clear();
    m_SubmeshesGPU.clear();
    m_TexturesGPU.clear();
}

// ====== CPU load ======

static inline bool HasTexCoords0(const aiMesh* m) { return m->mTextureCoords[0] != nullptr; }

void ModelResource::LoadCPU()
{
    m_MeshesCPU.clear();
    m_TexturesCPU.clear();
    m_MaterialsCPU.clear();
    m_TexIndexByPath.clear();

    std::string meshesDirectory = UPathFinder::Join(UPathFinder::ResolvePath(""), "Assets", "Meshes");
    const std::string absPath = UPathFinder::Join(meshesDirectory, m_Source);
    const std::string modelDir = UPathFinder::GetParent(absPath);

    Assimp::Importer importer;
    const aiScene* sc;
    sc = importer.ReadFile(
        absPath.c_str(),
        aiProcess_Triangulate
        | aiProcess_JoinIdenticalVertices
        | aiProcess_GenSmoothNormals
        | aiProcess_CalcTangentSpace
        | aiProcess_ImproveCacheLocality
        | aiProcess_SortByPType
        | aiProcess_OptimizeMeshes
        | aiProcess_OptimizeGraph
    );

    if (!sc || (sc->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !sc->mRootNode)
    {
        std::cerr << "[ModelResource] Assimp error: " << importer.GetErrorString() << "\n";
        return;
    }

    // --- Materials: only baseColor/diffuse for now ---
    m_MaterialsCPU.resize(sc->mNumMaterials);
    for (unsigned mi = 0; mi < sc->mNumMaterials; ++mi)
    {
        const aiMaterial* mat = sc->mMaterials[mi];
        FMatCPU mc{};

        // Try PBR baseColor, fall back to legacy diffuse
        aiString texPath;
        bool haveTex = false;
        if (mat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == aiReturn_SUCCESS)
            haveTex = true;
        else if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == aiReturn_SUCCESS)
            haveTex = true;

        if (haveTex)
        {
            // Embedded textures are possible (*0, *1) -> skip for MVP
            const char* p = texPath.C_Str();
            if (p && p[0] != '*')
            {
                std::string abs = UPathFinder::Normalize(modelDir + "/" + p);
                // Treat color as sRGB
                mc.baseColorTex = AcquireTexture(abs, /*srgb*/true);
            }
            else {
                // For now, ignore embedded. Later: decode embedded compressed blob.
                mc.baseColorTex = -1;
            }
        }
        m_MaterialsCPU[mi] = mc;
    }

    // --- Meshes: bake interleaved arrays ---
    std::function<void(aiNode*)> visit = [&](aiNode* n)
    {
        for (unsigned i = 0; i < n->mNumMeshes; ++i)
        {
            const aiMesh* m = sc->mMeshes[n->mMeshes[i]];
            const bool hasNormals  = m->HasNormals();
            const bool hasUVs      = HasTexCoords0(m);
            const bool hasTangents = m->HasTangentsAndBitangents();

            FMeshCPU out{};
            out.materialIndex = static_cast<int>(m->mMaterialIndex);
            out.vtx.reserve(m->mNumVertices * (3 + (hasNormals?3:0) + (hasUVs?2:0) + (hasTangents?3+3:0)));

            // Interleave
            for (unsigned v = 0; v < m->mNumVertices; ++v)
            {
                // pos
                out.vtx.push_back(m->mVertices[v].x);
                out.vtx.push_back(m->mVertices[v].y);
                out.vtx.push_back(m->mVertices[v].z);

                if (hasNormals) {
                    out.vtx.push_back(m->mNormals[v].x);
                    out.vtx.push_back(m->mNormals[v].y);
                    out.vtx.push_back(m->mNormals[v].z);
                }
                if (hasUVs) {
                    out.vtx.push_back(m->mTextureCoords[0][v].x);
                    out.vtx.push_back(m->mTextureCoords[0][v].y);
                }
                if (hasTangents) {
                    out.vtx.push_back(m->mTangents[v].x);
                    out.vtx.push_back(m->mTangents[v].y);
                    out.vtx.push_back(m->mTangents[v].z);

                    out.vtx.push_back(m->mBitangents[v].x);
                    out.vtx.push_back(m->mBitangents[v].y);
                    out.vtx.push_back(m->mBitangents[v].z);
                }
            }

            // indices
            out.idx.reserve(m->mNumFaces * 3);
            for (unsigned f = 0; f < m->mNumFaces; ++f) {
                const aiFace& face = m->mFaces[f];
                if (face.mNumIndices == 3) { // triangulated
                    out.idx.push_back(face.mIndices[0]);
                    out.idx.push_back(face.mIndices[1]);
                    out.idx.push_back(face.mIndices[2]);
                }
            }

            FillMeshUploadDesc(out, hasNormals, hasUVs, hasTangents);
            m_MeshesCPU.emplace_back(std::move(out));
        }

        for (unsigned c = 0; c < n->mNumChildren; ++c)
            visit(n->mChildren[c]);
    };
    visit(sc->mRootNode);

    m_CpuReady = true;
}

int ModelResource::AcquireTexture(const std::string& absPath, bool srgb)
{
    auto it = m_TexIndexByPath.find(absPath);
    if (it != m_TexIndexByPath.end())
        return it->second;

    // Load 4-channel RGBA8
    int w = 0, h = 0, comp = 0;
    unsigned char* data = stbi_load(absPath.c_str(), &w, &h, &comp, 4);
    if (!data) {
        std::cerr << "[ModelResource] Failed to load texture: " << absPath << "\n";
        m_TexIndexByPath[absPath] = -1;
        return -1;
    }

    FTexCPU tex{};
    tex.path = absPath;
    tex.w = w; tex.h = h;
    tex.channels = 4;
    tex.pixels.assign(data, data + (w * h * 4));
    stbi_image_free(data);

    // Upload descriptor (mipmaps recommended)
    tex.ro.type = ETexType::Tex2D;
    tex.ro.width = w;
    tex.ro.height = h;
    tex.ro.channels = 4;
    tex.ro.data = tex.pixels.data();
    tex.ro.format = srgb ? ETexFormat::SRGB8_A8 : ETexFormat::RGBA8;
    tex.ro.bGenerateMipmaps = true;
    tex.ro.wrapS = tex.ro.wrapT = ETexWrap::Repeat;
    tex.ro.minFilter = ETexFilter::LinearMipmapLinear;
    tex.ro.magFilter = ETexFilter::Linear;
    tex.ro.bSRGB = srgb;

    const int index = static_cast<int>(m_TexturesCPU.size());
    m_TexturesCPU.emplace_back(std::move(tex));
    m_TexIndexByPath[absPath] = index;
    return index;
}

void ModelResource::FillMeshUploadDesc(FMeshCPU& out, bool hasNormals, bool hasUVs, bool hasTangents)
{
    RMesh& ro = out.ro;
    ro.vertices = out.vtx;
    ro.indices  = out.idx;

    // Build attribute layout
    ro.attributes.clear();
    Rint offset = 0;
    const Rint floatSize = sizeof(float);

    // pos (location=0)
    ro.attributes.push_back( FVertexAttribute{ 0, offset, 3, /*type*/0, false} );
    offset += 3 * floatSize;

    if (hasNormals) {
        ro.attributes.push_back( FVertexAttribute{ 1, offset, 3, 0, false} );
        offset += 3 * floatSize;
        ro.bHasNormals = true;
    }
    if (hasUVs) {
        ro.attributes.push_back( FVertexAttribute{ 2, offset, 2, 0, false} );
        offset += 2 * floatSize;
        ro.bHasUVs = true;
    }
    if (hasTangents) {
        ro.attributes.push_back( FVertexAttribute{ 3, offset, 3, 0, false} );
        offset += 3 * floatSize;
        ro.attributes.push_back( FVertexAttribute{ 4, offset, 3, 0, false} );
        offset += 3 * floatSize;
        ro.bHasTangents = true;
    }
    ro.vertexStride = offset;
}

void ModelResource::UploadGPU()
{
    IRenderDevice* dev = GetDevice();
    if (!dev) {
        std::cerr << "[ModelResource] UploadGPU failed: device is null.\n";
        return;
    }

    // Textures
    m_TexturesGPU.resize(m_TexturesCPU.size());
    for (size_t i = 0; i < m_TexturesCPU.size(); ++i) {
        // ro.data must be valid at call time -> OK (still stored in m_TexturesCPU before ReleaseCPU)
        m_TexturesGPU[i] = dev->CreateTexture(m_TexturesCPU[i].ro);
    }

    // Materials
    m_MaterialsGPU.resize(m_MaterialsCPU.size());
    for (size_t i = 0; i < m_MaterialsCPU.size(); ++i) {
        FSurfaceDesc surf{};
        const int tIndex = m_MaterialsCPU[i].baseColorTex;
        if (tIndex >= 0 && static_cast<size_t>(tIndex) < m_TexturesGPU.size())
            surf.baseColor = m_TexturesGPU[tIndex];
        m_MaterialsGPU[i] = dev->CreateMaterial(surf);
    }

    // Meshes & submeshes
    m_SubmeshesGPU.clear();
    m_SubmeshesGPU.reserve(m_MeshesCPU.size());
    for (auto& m : m_MeshesCPU)
    {
        RMeshHandle mh = dev->CreateMesh(m.ro);
        RMaterialHandle mat = {};
        if (m.materialIndex >= 0 && static_cast<size_t>(m.materialIndex) < m_MaterialsGPU.size())
            mat = m_MaterialsGPU[m.materialIndex];

        m_SubmeshesGPU.push_back(FSubmeshGPU{ mh, mat });
    }
}

void ModelResource::ReleaseCPU()
{
    // Free CPU staging after GPU upload to save memory.
    m_MeshesCPU.clear();
    m_TexturesCPU.clear();
    m_MaterialsCPU.clear();
    m_TexIndexByPath.clear();
    m_CpuReady = false;
}

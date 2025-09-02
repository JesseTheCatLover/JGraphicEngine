// Copyright (c) 2025. JesseTheCatLover. All Rights Reserved.

#include "JModel.h"

#include "JShader.h"
#include <iostream>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

JModel::JModel(string Path)
{
    LoadModel(Path); // Load the model at construction
}

void JModel::Draw(JShader &Shader)
{
    // Draw all meshes
    for(unsigned int i = 0; i < Meshes.size(); i++)
        Meshes[i].Draw(Shader);
}

void JModel::LoadModel(string Path)
{
    Assimp::Importer Import;
    const aiScene* Scene = Import.ReadFile(
        (string(ENGINE_DIRECTORY) + "/Resources/Meshes/" + Path).c_str(),
        aiProcess_Triangulate |        // Ensure triangles
        aiProcess_FlipUVs |            // Flip UVs for OpenGL
        aiProcess_GenSmoothNormals |   // Generate normals if missing
        aiProcess_CalcTangentSpace     // Generate tangents/bitangents if missing
    );

    // Check for errors
    if(!Scene || Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !Scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP::" << Import.GetErrorString() << std::endl;
        return;
    }

    // Extract directory path
    Directory = Path.substr(0, Path.find_last_of('/'));
    ProcessNode(Scene->mRootNode, Scene);
}

void JModel::ProcessNode(aiNode *Node, const aiScene *Scene)
{
    // Process all meshes of this node
    for(unsigned int i = 0; i < Node->mNumMeshes; i++)
    {
        aiMesh* Mesh = Scene->mMeshes[Node->mMeshes[i]];
        Meshes.push_back(ProcessMesh(Mesh, Scene));
    }

    // Recursively process children
    for(unsigned int i = 0; i < Node->mNumChildren; i++)
        ProcessNode(Node->mChildren[i], Scene);
}

JMesh JModel::ProcessMesh(aiMesh *Mesh, const aiScene *Scene)
{
    std::cout << "Processing mesh " << Mesh->mName.C_Str() << " with " << Mesh->mNumVertices << " vertices\n";

    // Debug checks before reading:
    std::cout << "Has normals? " << (Mesh->HasNormals() ? "YES" : "NO") << "\n";
    std::cout << "Has tangents/bitangents? " << (Mesh->HasTangentsAndBitangents() ? "YES" : "NO") << "\n";
    std::cout << "Has texcoords[0]? " << (Mesh->mTextureCoords[0] ? "YES" : "NO") << "\n\n";

    std::vector<S_Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<S_Texture> textures;

    // Walk through each vertex
    for(unsigned int i = 0; i < Mesh->mNumVertices; i++)
    {
        S_Vertex Vertex{};
        // Position
        Vertex.Position = glm::vec3(Mesh->mVertices[i].x, Mesh->mVertices[i].y, Mesh->mVertices[i].z);

        // Normal (safe reading)
        Vertex.Normal = Mesh->HasNormals() ? glm::vec3(Mesh->mNormals[i].x, Mesh->mNormals[i].y, Mesh->mNormals[i].z) : glm::vec3(0.0f);

        // Texture coordinates (safe)
        Vertex.TexCoords = Mesh->mTextureCoords[0] ? glm::vec2(Mesh->mTextureCoords[0][i].x, Mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f);

        // Tangent / Bitangent (safe)
        if(Mesh->HasTangentsAndBitangents())
        {
            Vertex.Tangent   = glm::vec3(Mesh->mTangents[i].x, Mesh->mTangents[i].y, Mesh->mTangents[i].z);
            Vertex.Bitangent = glm::vec3(Mesh->mBitangents[i].x, Mesh->mBitangents[i].y, Mesh->mBitangents[i].z);
        }
        else
            Vertex.Tangent = Vertex.Bitangent = glm::vec3(0.0f);

        vertices.push_back(Vertex);
    }

    // Process indices
    for(unsigned int i = 0; i < Mesh->mNumFaces; i++)
    {
        const aiFace& face = Mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // Process material
    if(Mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = Scene->mMaterials[Mesh->mMaterialIndex];

        // Load diffuse, specular, normal, height maps
        auto DiffuseMaps  = LoadMaterialTextures(material, aiTextureType_DIFFUSE,  "texture_diffuse", Scene);
        auto SpecularMaps = LoadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", Scene);
        auto NormalMaps   = LoadMaterialTextures(material, aiTextureType_HEIGHT,   "texture_normal", Scene);
        auto HeightMaps   = LoadMaterialTextures(material, aiTextureType_AMBIENT,  "texture_height", Scene);

        textures.insert(textures.end(), DiffuseMaps.begin(), DiffuseMaps.end());
        textures.insert(textures.end(), SpecularMaps.begin(), SpecularMaps.end());
        textures.insert(textures.end(), NormalMaps.begin(), NormalMaps.end());
        textures.insert(textures.end(), HeightMaps.begin(), HeightMaps.end());
    }

    return JMesh(vertices, indices, textures);
}

std::vector<S_Texture> JModel::LoadMaterialTextures(aiMaterial* Mat, aiTextureType Type, std::string TypeName, const aiScene* Scene)
{
    std::vector<S_Texture> textures;

    for(unsigned int i = 0; i < Mat->GetTextureCount(Type); i++)
    {
        aiString str;
        Mat->GetTexture(Type, i, &str);

        // Check if texture is already loaded
        bool skip = false;
        for(const auto& loaded : TexturesLoaded)
        {
            if(std::strcmp(loaded.Path.data(), str.C_Str()) == 0)
            {
                textures.push_back(loaded);
                skip = true;
                break;
            }
        }
        if(skip) continue;

        // Load new texture
        S_Texture texture;
        texture.ID   = TextureFromFile(Scene, str, this->Directory);
        texture.Type = TypeName;
        texture.Path = str.C_Str();

        textures.push_back(texture);
        TexturesLoaded.push_back(texture);
    }

    return textures;
}

// Load texture from embedded data or external file
unsigned int JModel::TextureFromFile(const aiScene* Scene, const aiString& str, const std::string& directory)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    unsigned char* data = nullptr;
    int width, height, nrChannels = 0;

    // Embedded texture
    if(str.C_Str()[0] == '*')
    {
        int index = atoi(str.C_Str() + 1);
        const aiTexture* atex = Scene->mTextures[index];

        if(atex->mHeight == 0) // compressed format
            data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(atex->pcData), atex->mWidth, &width, &height, &nrChannels, 0);
        else
        {
            std::cout << "Unsupported raw texture format!" << std::endl;
            return 0;
        }
    }
    else // External file
    {
        std::string filename = std::string(ENGINE_DIRECTORY) + "/Resources/Meshes/" + directory + '/' + std::string(str.C_Str());
        data = stbi_load(filename.c_str(), &width, &height, &nrChannels, 0);
    }

    if(data)
    {
        GLenum format = GL_RGB;
        if(nrChannels == 1) format = GL_RED;
        else if(nrChannels == 3) format = GL_RGB;
        else if(nrChannels == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if(str.C_Str()[0] != '*') stbi_image_free(data); // free external texture data
    }
    else
    {
        std::cout << "Failed to load texture: " << str.C_Str() << std::endl;
        if(str.C_Str()[0] != '*') stbi_image_free(data);
    }

    return textureID;
}

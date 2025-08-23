// Copyright (c) 2024. JesseTheCatLover. All Rights Reserved.

#include "JModel.h"
#include "JShader.h"
#include <iostream>
#include <stb_image.h>

JModel::JModel(string Path)
{
    LoadModel(Path);
}

void JModel::Draw(JShader &Shader)
{
    for(unsigned int i = 0; i < Meshes.size(); i++)
        Meshes[i].Draw(Shader);
}

void JModel::LoadModel(string Path)
{
    Assimp::Importer Import;
    const aiScene* Scene = Import.ReadFile((string(ENGINE_DIRECTORY) + "/Resources/Meshes/" + Path).c_str(), aiProcess_Triangulate | aiProcess_FlipUVs);
    if(!Scene || Scene -> mFlags & AI_SCENE_FLAGS_INCOMPLETE || !Scene -> mRootNode)
    {
        cout << "ERROR::ASSIMP::" << Import.GetErrorString() << endl;
        return;
    }
    Directory = Path.substr(0, Path.find_last_of('/'));
    ProcessNode(Scene -> mRootNode, Scene);
}

void JModel::ProcessNode(aiNode *Node, const aiScene *Scene)
{
    // Process all nodes
    for(unsigned int i = 0; i < Node -> mNumMeshes; i++)
    {
        aiMesh* Mesh = Scene -> mMeshes[Node -> mMeshes[i]];
        Meshes.push_back(ProcessMesh(Mesh, Scene));
    }
    // For the children as well
    for(unsigned int i = 0; i < Node -> mNumChildren; i++)
    {
        ProcessNode(Node -> mChildren[i], Scene);
    }
}

JMesh JModel::ProcessMesh(aiMesh *Mesh, const aiScene *Scene)
{
    vector<S_Vertex> vertices;
    vector<unsigned int> indices;
    vector<S_Texture> textures;

    // Walk through each mesh's vertices
    for(unsigned int i = 0; i < Mesh -> mNumVertices; i++)
    {
        S_Vertex vertex;
        glm::vec3 vector; // Temp placeholder vector
        // Position
        vector.x = Mesh -> mVertices[i].x;
        vector.y = Mesh -> mVertices[i].y;
        vector.z = Mesh -> mVertices[i].z;
        vertex.Position = vector;
        // Normal
        if(Mesh -> HasNormals())
        {
            vector.x = Mesh -> mNormals[i].x;
            vector.y = Mesh -> mNormals[i].y;
            vector.z = Mesh -> mNormals[i].z;
            vertex.Normal = vector;
        }
        // Texture coordinates
        if(Mesh -> mTextureCoords[0]) // If mesh contains textcoords:
        {
            glm::vec2 vec2;
            vec2.x = Mesh -> mTextureCoords[0][i].x;
            vec2.y = Mesh -> mTextureCoords[0][i].y;
            vertex.TexCoords = vec2;
            // Tangent
            vector.x = Mesh -> mTangents[i].x;
            vector.y = Mesh -> mTangents[i].y;
            vector.z = Mesh -> mTangents[i].z;
            vertex.Tangent = vector;
            // Bitangent
            vector.x = Mesh -> mBitangents[i].x;
            vector.y = Mesh -> mBitangents[i].y;
            vector.z = Mesh -> mBitangents[i].z;
            vertex.Bitangent = vector;
        }
        else
            vertex.TexCoords = glm::vec2(0.f, 0.f);

        vertices.push_back(vertex);
    }

    // Walk through mesh's faces
    for(unsigned int i = 0; i < Mesh -> mNumFaces; i++)
    {
        aiFace face = Mesh -> mFaces[i];
        // Retrieve indices
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // Process material
    if(Mesh -> mMaterialIndex >= 0)
    {
        aiMaterial* material = Scene -> mMaterials[Mesh -> mMaterialIndex];

        // Diffuse maps
        vector<S_Texture> DiffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), DiffuseMaps.begin(), DiffuseMaps.end());

        // Specular maps
        vector<S_Texture> SpecularMaps = LoadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), SpecularMaps.begin(), SpecularMaps.end());

        // Normal maps
        vector<S_Texture> NormalMaps = LoadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), NormalMaps.begin(), NormalMaps.end());

        // Height maps
        vector<S_Texture> HeightMaps = LoadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), HeightMaps.begin(), HeightMaps.end());
    }

    return JMesh(vertices, indices, textures);
}

vector<S_Texture> JModel::LoadMaterialTextures(aiMaterial *Mat, aiTextureType Type, string TypeName)
{
    vector<S_Texture> textures;
    for(unsigned int i = 0; i < Mat ->GetTextureCount(Type); i++)
    {
        aiString str;
        Mat -> GetTexture(Type, i, &str);
        // Check if texture was loaded before, (Should skip?):
        bool skip = false;
        for(unsigned int j = 0; j < TexturesLoaded.size(); j++)
        {
            if(std::strcmp(TexturesLoaded[j].Path.data(), str.C_Str()) == 0)
            {
                textures.push_back(TexturesLoaded[j]);
                skip = true; break; // a texture with the same filepath has already been loaded, skip this one.
            }
        }
        if(!skip) // If texture hasn't been loaded already, load it.
        {
            S_Texture texture;
            texture.ID = TextureFromFile(str.C_Str(), this->Directory);
            texture.Type = TypeName;
            texture.Path = str.C_Str();

            textures.push_back(texture);
            TexturesLoaded.push_back(texture); // Ensure we won't load duplicate textures.
        }
    }
    return textures;
}

unsigned int JModel::TextureFromFile(const char* path, const string &directory, bool gamma)
{
    string filename = string(path);
    filename = directory + '/' + filename;

    unsigned int TextureID;
    glGenTextures(1, &TextureID);

    int width, height, nrComponent;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponent, 0);
    if(data)
    {
        GLenum format;
        if(nrComponent == 1)
            format = GL_RED;
        else if(nrComponent == 3)
            format = GL_RGB;
        else if(nrComponent == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, TextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        cout << "Texture failed to load at path: " << path << endl;
        stbi_image_free(data);
    }

    return TextureID;
}

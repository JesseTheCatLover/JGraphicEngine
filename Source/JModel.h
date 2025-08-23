// Copyright (c) 2024. JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <string>
#include "JMesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace std;

class JModel
{
public:
    JModel(string Path);

    vector<S_Texture> TexturesLoaded;
    vector<JMesh> Meshes;
    string Directory;

    void Draw(class JShader &Shader);

private:
    void LoadModel(string Path);
    void ProcessNode(aiNode* Node, const aiScene* Scene);
    class JMesh ProcessMesh(aiMesh* Mesh, const aiScene* Scene);
    vector<S_Texture> LoadMaterialTextures(aiMaterial* Mat, aiTextureType Type, string TypeName);
    unsigned int TextureFromFile(const char* path, const string &directory, bool gamma = false);
};

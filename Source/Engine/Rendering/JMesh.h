// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

#define MAX_BONE_INFLUENCE 4

using namespace std;
using namespace glm;

struct S_Vertex
{
    vec3 Position;
    vec3 Normal;
    vec2 TexCoords;
    vec3 Tangent;
    vec3 Bitangent;
    int M_BoneIDs[MAX_BONE_INFLUENCE]; // Bone indexes which will influence this vertex
    float M_Weights[MAX_BONE_INFLUENCE]; // Weights from each bone
};

struct S_Texture
{
    unsigned int ID;
    string Type;
    string Path;
};

class JMesh
{
public:
    vector<S_Vertex> Vertices;
    vector<unsigned int> Indices;
    vector<S_Texture> Textures;
    unsigned int VAO;

    JMesh(vector<S_Vertex> Vertices, vector<unsigned int> Indices, vector<S_Texture> Textures);

    void Draw(class JShader &Shader);

private:
    unsigned int VBO, EBO;

    void SetupMesh();
};

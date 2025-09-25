// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

#include "glm/fwd.hpp"
#include "glm/vec3.hpp"

class JShader;
class JModel;

class JActor {
public:
    std::string Name;
    unsigned int ID;
    size_t m_VectorIndex;
    JModel *Model;
    glm::vec3 Position;
    glm::vec3 Rotation;
    glm::vec3 Scale;

    struct S_JActorRenderConfig {
        bool bIsTransparent = false;     // Opaque or Transparent
        bool bDrawOutline = false;       // whether to render an outline
        float OutlineThickness = 0.03f;  // how thick the outline should be
        bool bWireframe = false;         // optional: wireframe mode
        bool bBackCulling = false;       // whether to cull back faces
    };

    S_JActorRenderConfig Config;

    JActor(JModel *model, std::string name,glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
        : Model(model), Name(name), Position(position), Rotation(rotation), Scale(scale) {}
    JActor(JModel *model, std::string name, glm::vec3 position)
        : Model(model), Name(name), Position(position), Scale(glm::vec3(1.f)) {}
    JActor(JModel *model, std::string name)
        : Model(model), Name(name), Scale(glm::vec3(1.f)) {}

    glm::mat4 GetModelMatrix() const;

    void Draw(JShader& shader) const;
    void DrawConfig(JShader& shader, JShader& outlineShader) const;
};

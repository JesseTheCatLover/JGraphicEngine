// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "glm/fwd.hpp"
#include "glm/vec3.hpp"

class JShader;
class JModel;

class JActor {
public:
    JModel *Model;
    glm::vec3 Position;
    glm::vec3 Rotation;
    glm::vec3 Scale;

    struct S_JActorRenderConfig {
        bool bDrawOutline = false;       // whether to render an outline
        float OutlineThickness = 0.03f;  // how thick the outline should be
        bool bWireframe = false;         // optional: wireframe mode
    };

    S_JActorRenderConfig Config;

    JActor(JModel *model, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
        : Model(model), Position(position), Rotation(rotation), Scale(scale) {}
    JActor(JModel *model, glm::vec3 position)
        : Model(model), Position(position), Scale(glm::vec3(1.f)) {}
    JActor(JModel *model)
        : Model(model), Scale(glm::vec3(1.f)) {}

    glm::mat4 GetModelMatrix() const;

    void Draw(JShader& shader) const;
    void DrawConfig(JShader& shader, JShader& outlineShader) const;
};

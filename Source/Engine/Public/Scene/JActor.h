//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include "Core/JCoreObject.h"
#include "Core/Serialization/JsonWritter.h"
#include "Core/Serialization/JsonReader.h"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

class JShader;
class JModel;


struct FActorConfig
{
    bool bDrawOutline = false;
    bool bBackCulling = false;
    float outlineThickness = 0.05f;

    void Serialize(JsonWriter& writer) const {
        writer.Write("draw_outline", bDrawOutline);
        writer.Write("back_culling", bBackCulling);
        writer.Write("outline_thickness", outlineThickness);
    }

    void Deserialize(JsonReader& reader) {
        bDrawOutline = reader.Read("draw_outline", false);
        bBackCulling = reader.Read("back_culling", false);
        outlineThickness = reader.Read("outline_thickness", 0.05f);
    }
};

class JActor : public JCoreObject
{
    DECLARE_JOBJECT(JActor)

public:
    // Actor metadata
    std::string Name;
    size_t m_VectorIndex = 0;

    // Transform
    glm::vec3 Position {0.0f};
    glm::vec3 Rotation {0.0f};
    glm::vec3 Scale    {1.0f};

    // Renderable link
    JModel* Model = nullptr;

    FActorConfig Config;

    // Constructors
    JActor() = default;
    JActor(JModel* model, const std::string& name,
           glm::vec3 position = {0,0,0},
           glm::vec3 rotation = {0,0,0},
           glm::vec3 scale    = {1,1,1})
        : Model(model), Name(name), Position(position), Rotation(rotation), Scale(scale) {}

    // Engine interface
    glm::mat4 GetModelMatrix() const;

    void Draw(JShader& shader) const;
    void DrawConfig(JShader& shader, JShader& outlineShader) const;

    // Serialization
    void Serialize(class JsonWriter& writer) const override;
    void Deserialize(const class JsonReader& reader) override;
};

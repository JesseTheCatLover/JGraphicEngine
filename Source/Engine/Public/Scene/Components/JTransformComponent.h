//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "JComponent.h"

#include "glm/vec3.hpp"

class JTransformComponent : public JComponent
{
    DECLARE_JOBJECT(JTransformComponent);

public:
    glm::vec3 Position{0.0f};
    glm::vec3 Rotation{0.0f};
    glm::vec3 Scale{1.0f};

protected:
    void SerializeProperties(JsonWriter& Writer) const override;
    void DeserializeProperties(const JsonReader& Reader) override;
};

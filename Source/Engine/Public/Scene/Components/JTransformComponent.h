//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "JComponent.h"

#include "glm/fwd.hpp"
#include "glm/vec3.hpp"

class JTransformComponent : public JComponent
{
    DECLARE_JOBJECT(JTransformComponent, JComponent);

public:
    glm::vec3 Position{0.0f};
    glm::vec3 Rotation{0.0f};
    glm::vec3 Scale{1.0f};

    glm::mat4 GetLocalTransform() const;

protected:
    void SerializeProperties(JsonWriter& Writer) const override;
    void DeserializeProperties(const JsonReader& Reader) override;
};

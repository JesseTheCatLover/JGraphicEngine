//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Scene/Components/SceneComponents/JSceneComponent.h"
#include <string>
#include <memory>

class JShader;
class JModelResource;

/**
 * @class JModelComponent
 * @brief Component responsible for rendering a model within the scene.
 *
 * Inherits from JSceneComponent, so it has full transform, hierarchy, and attachment support.
 * This component holds a reference to a JModelResource, which encapsulates the mesh and
 * any associated materials or textures.
 *
 * Responsibilities:
 *  - Store and reference the model resource.
 *  - Provide transform and world-space information through the scene graph.
 *  - Support serialization of its resource reference and transform state.
 *
 * Example usage:
 * @code
 * auto modelComp = Actor->AddComponent<JModelComponent>();
 * modelComp->SetModel(ResourceManager.Load<JModelResource>("Models/Tree.fbx", "Models/Tree.fbx"));
 * @endcode
 */

class JModelComponent : public JSceneComponent
{
    DECLARE_JOBJECT(JModelComponent, JSceneComponent)

private:
    std::string m_ModelPath; ///< Serialized path/key of the model
    std::weak_ptr<JModelResource> m_ModelResource; ///< Loaded model resource

public:
    JModelComponent() = default;
    virtual ~JModelComponent() = default;

    /**
     * @brief Set the model resource by path/key.
     * @param InPath Path or key to the model resource.
     */
    void SetModel(const std::string& InPath);

    /**
     * @brief Get the model resource.
     * @return Pointer to the model resource.
     */
    std::shared_ptr<JModelResource> GetModel() const { return m_ModelResource.lock(); }

    /**
     * @brief Draw the mesh using the provided shader.
     * @param Shader Shader to use for rendering.
     */
    void Draw(JShader& Shader) const;

protected:
    void SerializeProperties(JsonWriter& Writer) const override;
    void DeserializeProperties(const JsonReader& Reader) override;
};

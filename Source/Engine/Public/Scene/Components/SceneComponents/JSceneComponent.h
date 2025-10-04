//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Scene/Components/JTransformComponent.h"
#include <vector>
#include "glm/matrix.hpp"

/**
 * @class JSceneComponent
 * @brief A transformable component that supports hierarchical attachment.
 *
 * Scene components form the basis of the scene graph. They extend transform
 * functionality with parent/child relationships. This allows components to be
 * positioned relative to each other.
 */
class JSceneComponent : public JTransformComponent
{
    DECLARE_JOBJECT(JSceneComponent, JTransformComponent)

protected:
    // Hierarchy
    JSceneComponent* m_Parent = nullptr; ///< Parent component in the hierarchy
    std::vector<JSceneComponent*> m_Children; ///< Child components

public:
    JSceneComponent() = default;
    virtual ~JSceneComponent() = default;

    /**
     * @brief Attach this component to a parent component.
     *
     * @param parent The component to attach to. Pass nullptr to detach.
     */
    void AttachToComponent(JSceneComponent* parent);

    /**
     * @brief Detach this component from its current parent.
     */
    void Detach();

    /**
     * @brief Get the parent of this component.
     * @return Pointer to the parent scene component, or nullptr if root.
     */
    JSceneComponent* GetParent() const { return m_Parent; }

    /**
     * @brief Get all children attached to this component.
     * @return Const reference to children vector.
     */
    const std::vector<JSceneComponent*>& GetChildren() const { return m_Children; }

    /** @brief Set the owning actor for this component. */
    void SetOwnerActor(JActor* actor) { m_OwnerActor = actor; }

    /** @brief Get the owning actor. */
    JActor* GetOwnerActor() const { return m_OwnerActor; }

    /**
     * @brief Compute the world transform of this component.
     *
     * Combines local transform with all parents recursively.
     *
     * @return World transform matrix.
     */
    glm::mat4 GetWorldTransform() const;

protected:
    void OnAttachment() override;

    /** @brief Serialize component-specific properties, including transform and hierarchy. */
    void SerializeProperties(JsonWriter& Writer) const override;

    /** @brief Deserialize component-specific properties, including transform and hierarchy. */
    void DeserializeProperties(const JsonReader& Reader) override;

};

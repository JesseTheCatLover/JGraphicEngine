//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Scene/ActorComponents/JTransformComponent.h"
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

private:
    friend class JActor;

    /**
     * @brief Private helper to detach this component from its parent without reparenting it to the actor's root.
     * Making it a non-owned component.
     *
     * After calling this:
     *  - The parent will no longer reference this component.
     *  - This component will not belong to any hierarchy.
     *  - Children remain linked to this component
     */
    void UnlinkFromParent();

protected:
    // Hierarchy
    JSceneComponent* m_Parent = nullptr; ///< Parent component in the hierarchy
    std::vector<JSceneComponent*> m_Children; ///< Child components

    void MarkWorldDirty();

    mutable FTransform m_WorldTransform;
    mutable bool m_WorldDirty;

public:
    JSceneComponent(): m_WorldDirty(true) {};
    virtual ~JSceneComponent() = default;

    /**
     * @brief Attach this component to a parent component.
     *
     * @param parent The component to attach to. Pass nullptr to detach.
     */
    void AttachToComponent(JSceneComponent* parent);

    /**
     * @brief Detach this component from its current scene component parent,
     * and re attach it to the actor's root component.
     */
    void Detach();

    bool DestroyComponent() final;

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

    /**
    * @brief Compute the world transform of this component.
    *
    * Combines local transform with all parents recursively. This ensures
    * the returned transform is in world space, respecting hierarchical attachments.
    *
    * @return World transform as an FTransform.
    */
    [[nodiscard]] FTransform GetWorldTransform() const;

    /**
     * @brief Compute the world position of this component.
     * @return Position vector in world space.
     */
    [[nodiscard]] FVector3 GetWorldPosition() const { return GetWorldTransform().GetPosition(); }

    /**
    * @brief Compute the world rotation of this component as quaternion.
    * @return World rotation quaternion.
    */
    [[nodiscard]] FQuat GetWorldRotationAsQuat() const { return GetWorldTransform().GetRotation(); }

    /**
    * @brief Compute the world rotation of this component as FRotator.
    * @return World rotation in euler degrees.
    */
    [[nodiscard]] FRotator GetWorldRotationAsRotator() const { return GetWorldTransform().GetRotationAsRotator(); }

    /**
     * @brief Compute the world rotation of this component as FEuler (radians internally).
     * @return World rotation.
     */
    [[nodiscard]] FEuler GetWorldRotationAsEuler() const { return GetWorldTransform().GetRotation().ToEuler(); }

protected:
    void PostLoad() override;

public:
    /**
    * @brief Set world position, adjusting local transform accordingly.
    */
    void SetWorldPosition(const FVector3& worldPosition);

    /**
     * @brief Updates the world-space position of the component.
     * @param x The new world X position.
     * @param y The new world Y position.
     * @param z The new world Z position.
     */
    void SetWorldPosition(float x, float y, float z)
    {
        SetWorldPosition(FVector3(x, y, z));
    }

    /**
     * @brief Set world rotation (as quaternion), adjusting local transform accordingly.
     */
    void SetWorldRotation(const FQuat& worldRotation);

    /**
     * @brief Set world rotation (using FRotator), adjusting local transform accordingly.
     */
    void SetWorldRotation(const FRotator& worldRotation);

    /**
     * @brief Set world scale, adjusting local transform accordingly.
     */
    void SetWorldScale(const FVector3& worldScale);

    /**
     * @brief Set full world transform, adjusting local transform accordingly.
     */
    void SetWorldTransform(const FTransform& worldTransform);

protected:
    /** @brief Called when the component is attached to a parent actor. */
    void OnAttachment() override;

    /**
     * @brief Called whenever the local transform changes (position, rotation, scale).
     * Propagates a world-dirty state through the hierarchy.
     */
    void OnLocalTransformChanged() override
    {
        MarkWorldDirty();
    }

    virtual void OnWorldTransformChanged() {}

    void BeginPlay() override;

    void EndPlay() override;

    void OnDestroy() override;

public:
    void Tick(float deltaTime) override;

protected:
    void Initialize() override;
};

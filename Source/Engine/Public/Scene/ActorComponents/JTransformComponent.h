// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Scene/ActorComponents/JActorComponent.h"
#include "Core/Math/FTransform.h"
#include "JTransformComponent.generated.h"

/**
 * @class JTransformComponent
 * @brief Provides position, rotation, and scale functionality for actors.
 *
 * This component defines a local-space transform for its owning actor,
 * storing translation, rotation, and scale. It is the foundation for all
 * scene-related components (e.g., mesh, camera, light) that need spatial data.
 */
JCLASS()
class JTransformComponent : public JActorComponent
{
    GENERATED_BODY()

private:
    /** Local transform storing position, rotation, and scale. */
    JPROPERTY(HiddenToInspector)
    FTransform m_LocalTransform;

public:
    JTransformComponent() = default;

    //==================================================
    // Position
    //==================================================

    /**
     * @brief Returns the current local-space position of the component.
     * @return A copy of the local position vector.
     */
    [[nodiscard]] FVector3 GetLocalPosition() const { return m_LocalTransform.GetPosition(); }

    /**
     * @brief Updates the local-space position of the component.
     * @param position The new local position.
     */
    void SetLocalPosition(const FVector3& position)
    {
        m_LocalTransform.SetPosition(position);
        OnLocalTransformChanged();
    }

    /**
     * @brief Updates the local-space position of the component.
     * @param x The new local X position.
     * @param y The new local Y position.
     * @param z The new local Z position.
     */
    void SetLocalPosition(float x, float y, float z)
    {
        SetLocalPosition(FVector3(x, y, z));
    }

    //==================================================
    // Rotation
    //==================================================

    /**
     * @brief Returns the local rotation represented as a quaternion.
     * @return The local-space rotation quaternion.
     */
    [[nodiscard]] FQuat GetLocalRotationAsQuat() const { return m_LocalTransform.GetRotation(); }

    /**
     * @brief Returns the local rotation represented as an FRotator (in degrees).
     * @return The local-space rotation as an FRotator.
     */
    [[nodiscard]] FRotator GetLocalRotationAsRotator() const { return m_LocalTransform.GetRotation().ToRotator(); }

    /**
     * @brief Returns the local rotation represented as an FEuler (in radians).
     * @return The local-space rotation as an FEuler.
     */
    [[nodiscard]] FEuler GetLocalRotationAsEuler() const { return m_LocalTransform.GetRotation().ToEuler(); }

    /**
     * @brief Sets the local rotation using an FRotator (degrees).
     * @param rotator The new rotation.
     */
    void SetLocalRotation(const FRotator& rotator)
    {
        SetLocalRotation(FQuat::MakeFromRotator(rotator));
    }

    /**
     * @brief Sets the local rotation using a quaternion.
     * @param rotation The new rotation quaternion.
     */
    void SetLocalRotation(const FQuat& rotation)
    {
        m_LocalTransform.SetRotation(rotation);
        OnLocalTransformChanged();
    }

    /**
     * @brief Sets the local rotation using Euler angles.
     * @param euler The new rotation in radians.
     */
    void SetLocalRotation(const FEuler& euler)
    {
        m_LocalTransform.SetRotation(euler);
        OnLocalTransformChanged();
    }

    //==================================================
    // Scale
    //==================================================

    /**
     * @brief Returns the local scale of the component.
     * @return A copy of the local scale vector.
     */
    [[nodiscard]] FVector3 GetLocalScale() const { return m_LocalTransform.GetScale(); }

    /**
     * @brief Updates the local scale of the component.
     * @param scale The new local scale.
     */
    void SetLocalScale(const FVector3& scale)
    {
        m_LocalTransform.SetScale(scale);
        OnLocalTransformChanged();
    }

    //==================================================
    // Transform
    //==================================================

    /**
     * @brief Returns the complete local transform (position, rotation, scale).
     * @return A constant reference to the transform data.
     */
    [[nodiscard]] const FTransform& GetLocalTransform() const { return m_LocalTransform; }

    /**
     * @brief Updates the entire local transform at once.
     * @param transform The new transform.
     */
    void SetLocalTransform(const FTransform& transform)
    {
        m_LocalTransform = transform;
        OnLocalTransformChanged();
    }

    void Tick(float deltaTime) override {}

protected:
    /**
     * @brief Called whenever the local transform changes (position, rotation, or scale).
     *
     * Override this in subclasses (e.g., JSceneComponent) to respond
     * to local-space updates such as matrix recalculation or hierarchy propagation.
     */
    virtual void OnLocalTransformChanged() {}

    void OnAttachment() override {}

    void Initialize() override {}
};
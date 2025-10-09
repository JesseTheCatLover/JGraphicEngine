//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Scene/Components/JActorComponent.h"
#include "Core/Math/FTransform.h"

/**
 * @class JTransformComponent
 * @brief Local transform component for actors.
 */
class JTransformComponent : public JActorComponent
{
    DECLARE_JOBJECT(JTransformComponent, JActorComponent);

private:
    /** Local transform storing position, rotation, and scale. */
    FTransform m_LocalTransform;

public:
    //==================================================
    // Position
    //==================================================

    /**
     * @brief Returns the local position of the component.
     * @return Reference to the local position vector.
     */
    [[nodiscard]] const FVector3& GetLocalPosition() const { return m_LocalTransform.GetPosition(); }

    /**
     * @brief Sets the local position of the component.
     * @param position New local position vector.
     */
    void SetLocalPosition(const FVector3& position)
    {
        m_LocalTransform.SetPosition(position);
        OnLocalTransformChanged();
    }

    //==================================================
    // Rotation
    //==================================================

    /**
     * @brief Returns the local rotation as a quaternion.
     * @return Constant reference to the quaternion representing rotation.
     */
    [[nodiscard]] const FQuat& GetLocalRotationAsQuat() const { return m_LocalTransform.GetRotation(); }

    [[nodiscard]] FRotator GetLocalRotationAsRotator() const { return m_LocalTransform.GetRotation().ToRotator(); }

    /**
     * @brief Returns the local rotation as an FEuler object.
     * @return FEuler containing rotation in radians internally.
     */
    [[nodiscard]] const FEuler& GetLocalRotationAsEuler() const { return m_LocalTransform.GetRotation().ToEuler(); }

    /**
     * @brief Sets the local rotation using a FRotator.
     * @param rotator New rotation represented as FRotator.
     */
    void SetLocalRotation(const FRotator& rotator)
    {
        SetLocalRotation(FQuat::MakeFromRotator(rotator));
    }

    /**
     * @brief Sets the local rotation using a quaternion.
     * @param rotation New rotation quaternion.
     */
    void SetLocalRotation(const FQuat& rotation)
    {
        m_LocalTransform.SetRotation(rotation);
        OnLocalTransformChanged();
    }

    /**
     * @brief Sets the local rotation using an FEuler object.
     * @param euler FEuler rotation to apply.
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
     * @return Reference to the local scale vector.
     */
    [[nodiscard]] const FVector3& GetLocalScale() const { return m_LocalTransform.GetScale(); }

    /**
     * @brief Sets the local scale of the component.
     * @param scale New local scale vector.
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
     * @brief Returns the full local transform of the component.
     */
    [[nodiscard]] const FTransform& GetLocalTransform() const { return m_LocalTransform; }

    /**
     * @brief Sets the entire local transform at once.
     */
    void SetLocalTransform(const FTransform& transform)
    {
        m_LocalTransform = transform;
        OnLocalTransformChanged();
    }

protected:
    /**
     * @brief Called whenever the local transform changes (position, rotation, or scale).
     *
     * Subclasses such as JSceneComponent should override this to react to local transform updates.
     */
    virtual void OnLocalTransformChanged() {}

    void SerializeProperties(JsonWriter& writer) const override;
    void DeserializeProperties(const JsonReader& reader) override;
};

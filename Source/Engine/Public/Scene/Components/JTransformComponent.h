//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "JComponent.h"
#include "Core/Math/FTransform.h"

/**
 * @class JTransformComponent
 * @brief Local transform component for actors.
 */
class JTransformComponent : public JComponent
{
    DECLARE_JOBJECT(JTransformComponent, JComponent);

public:
    /** Local transform storing position, rotation, and scale. */
    FTransform LocalTransform;

    //==================================================
    // Position
    //==================================================

    /**
     * @brief Returns the local position of the component.
     * @return Reference to the local position vector.
     */
    [[nodiscard]] const FVector3& GetPosition() const { return LocalTransform.position(); }

    /**
     * @brief Sets the local position of the component.
     * @param Pos New local position vector.
     */
    void SetPosition(const FVector3& Pos) { LocalTransform.position() = Pos; }

    //==================================================
    // Rotation
    //==================================================

    /**
     * @brief Returns the local rotation as Euler angles in degrees.
     * @return Rotation vector (Pitch=X, Yaw=Y, Roll=Z) in degrees.
     */
    [[nodiscard]] FVector3 GetRotation() const { return GetEulerRotation().ToVector3(); }

    /**
     * @brief Returns the local rotation as Euler angles in radians.
     * @return Rotation vector (Pitch=X, Yaw=Y, Roll=Z) in radians.
     */
    [[nodiscard]] FVector3 GetRotationRadian() const { return FMath::Radians(GetRotation()); }

    /**
     * @brief Returns the local rotation as a quaternion.
     * @return Constant reference to the quaternion representing rotation.
     */
    [[nodiscard]] const FQuat& GetQuatRotation() const { return LocalTransform.rotation(); }

    /**
     * @brief Sets the local rotation using a quaternion.
     * @param Rot New rotation quaternion.
     */
    void SetQuatRotation(const FQuat& Rot) { LocalTransform.rotation() = Rot; }

    /**
     * @brief Returns the local rotation as an FEuler object.
     * @return FEuler containing rotation in radians internally.
     */
    [[nodiscard]] FEuler GetEulerRotation() const { return LocalTransform.rotation().ToEuler(); }

    /**
     * @brief Sets the local rotation using an FEuler object.
     * @param Euler FEuler rotation to apply (internally converted to quaternion).
     */
    void SetEulerRotation(const FEuler& Euler) { LocalTransform.rotation() = Euler.ToQuat(); }

    //==================================================
    // Scale
    //==================================================

    /**
     * @brief Returns the local scale of the component.
     * @return Reference to the local scale vector.
     */
    [[nodiscard]] const FVector3& GetScale() const { return LocalTransform.scale(); }

    /**
     * @brief Sets the local scale of the component.
     * @param S New local scale vector.
     */
    void SetScale(const FVector3& S) { LocalTransform.scale() = S; }

    //==================================================
    // Transform
    //==================================================

    /**
     * @brief Returns the local transformation matrix of the component.
     * @return FMatrix representing the combined local transform (T * R * S).
     */
    [[nodiscard]] FMatrix GetLocalTransform() const { return LocalTransform.ToMatrix(); }

protected:
    void SerializeProperties(JsonWriter& Writer) const override;
    void DeserializeProperties(const JsonReader& Reader) override;
};

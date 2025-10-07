// Copyright 2025 JesseTheCatLover. All Rights Reserved.
#pragma once

#include <sstream>
#include <string>

/**
 * @struct FVector3
 * @brief Represents a 3D vector with X, Y, and Z components.
 *
 * Provides basic arithmetic operations, normalization, interpolation,
 * and geometric utilities such as dot and cross products.
 */
struct FVector3
{
    float x, y, z;

    /** Default constructor. Initializes all components to zero. */
    constexpr FVector3() : x(0), y(0), z(0) {}

    /** Constructs a vector with the given X, Y, and Z values. */
    constexpr FVector3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}

    /** Constructs a vector with all components equal to the given scalar. */
    explicit constexpr FVector3(float Scalar) : x(Scalar), y(Scalar), z(Scalar) {}

    /** @return The length (magnitude) of the vector. */
    [[nodiscard]] float Length() const { return std::sqrt(x * x + y * y + z * z); }

    /**
     * @brief Returns a normalized copy of the vector.
     * @return A unit-length vector pointing in the same direction.
     */
    [[nodiscard]] FVector3 Normalized() const
    {
        float len = Length();
        if (len == 0.0f)
            return FVector3(0.0f);
        float inv = 1.0f / len;
        return { x * inv, y * inv, z * inv };
    }

    /**
     * @brief Computes the dot product with another vector.
     * @param Other The vector to dot against.
     * @return The scalar dot product value.
     */
    [[nodiscard]] float Dot(const FVector3& Other) const { return x * Other.x + y * Other.y + z * Other.z; }

    /**
     * @brief Computes the cross product with another vector.
     * @param Other The vector to cross with.
     * @return The cross product vector (perpendicular to both).
     */
    [[nodiscard]] FVector3 Cross(const FVector3& Other) const
    {
        return {
            y * Other.z - z * Other.y,
            z * Other.x - x * Other.z,
            x * Other.y - y * Other.x
        };
    }

    /**
     * @brief Computes the distance between this vector and another.
     * @param Other The target vector.
     * @return The scalar distance between the two points.
     */
    [[nodiscard]] float Distance(const FVector3& Other) const
    {
        float dx = x - Other.x;
        float dy = y - Other.y;
        float dz = z - Other.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    /**
     * @brief Linearly interpolates between this vector and another.
     * @param B The target vector.
     * @param Alpha Interpolation factor (0.0 to 1.0).
     * @return The interpolated vector.
     */
    [[nodiscard]] FVector3 Lerp(const FVector3& B, float Alpha) const
    {
        return {
            x * (1.0f - Alpha) + B.x * Alpha,
            y * (1.0f - Alpha) + B.y * Alpha,
            z * (1.0f - Alpha) + B.z * Alpha
        };
    }

    /** @name Arithmetic Operators */
    ///@{

    /** Adds two vectors component-wise. */
    constexpr FVector3 operator+(const FVector3& Other) const { return { x + Other.x, y + Other.y, z + Other.z }; }

    /** Subtracts two vectors component-wise. */
    constexpr FVector3 operator-(const FVector3& Other) const { return { x - Other.x, y - Other.y, z - Other.z }; }

    /** Adds another vector to this one in place. */
    FVector3& operator+=(const FVector3& Other)
    {
        x += Other.x;
        y += Other.y;
        z += Other.z;
        return *this;
    }

    /** Subtracts another vector from this one in place. */
    FVector3& operator-=(const FVector3& Other)
    {
        x -= Other.x;
        y -= Other.y;
        z -= Other.z;
        return *this;
    }

    /** Multiplies each component by a scalar. */
    constexpr FVector3 operator*(float Scalar) const { return { x * Scalar, y * Scalar, z * Scalar }; }

    /** Divides each component by a scalar. */
    constexpr FVector3 operator/(float Scalar) const { return { x / Scalar, y / Scalar, z / Scalar }; }

    /** Multiplies this vector by a scalar in place. */
    FVector3& operator*=(float Scalar)
    {
        x *= Scalar;
        y *= Scalar;
        z *= Scalar;
        return *this;
    }

    /** Divides this vector by a scalar in place. */
    FVector3& operator/=(float Scalar)
    {
        x /= Scalar;
        y /= Scalar;
        z /= Scalar;
        return *this;
    }

    /** Unary negation operator. Returns the inverse of the vector. */
    constexpr FVector3 operator-() const { return { -x, -y, -z }; }

    ///@}

    /** @name Comparison Operators */
    ///@{

    /** Checks if two vectors are approximately equal. */
    bool operator==(const FVector3& Other) const
    {
        const float Epsilon = 1e-6f;
        return (std::fabs(x - Other.x) < Epsilon) &&
               (std::fabs(y - Other.y) < Epsilon) &&
               (std::fabs(z - Other.z) < Epsilon);
    }

    /** Checks if two vectors are not approximately equal. */
    bool operator!=(const FVector3& Other) const { return !(*this == Other); }

    ///@}

    /** Component-wise multiplication with another vector */
    constexpr FVector3 operator*(const FVector3& Other) const
    {
        return { x * Other.x, y * Other.y, z * Other.z };
    }

    /** Component-wise multiplication assignment */
    FVector3& operator*=(const FVector3& Other)
    {
        x *= Other.x;
        y *= Other.y;
        z *= Other.z;
        return *this;
    }

    /** Component-wise division with another vector */
    constexpr FVector3 operator/(const FVector3& Other) const
    {
        return { x / Other.x, y / Other.y, z / Other.z };
    }

    /** Component-wise division assignment */
    FVector3& operator/=(const FVector3& Other)
    {
        x /= Other.x;
        y /= Other.y;
        z /= Other.z;
        return *this;
    }

    /**
     * @brief Allows scalar multiplication from the left-hand side.
     * @param Scalar The scalar value.
     * @param Vec The vector to scale.
     * @return The scaled vector.
     */
    friend constexpr FVector3 operator*(float Scalar, const FVector3& Vec)
    {
        return { Vec.x * Scalar, Vec.y * Scalar, Vec.z * Scalar };
    }

    /**
     * @brief Converts the vector to a string (formatted as "x y z").
     * @return The string representation of the vector.
     */
    [[nodiscard]] std::string ToString() const
    {
        std::ostringstream ss;
        ss << x << " " << y << " " << z;
        return ss.str();
    }
};

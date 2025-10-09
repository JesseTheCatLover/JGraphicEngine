// Copyright 2025 JesseTheCatLover. All Rights Reserved.
#pragma once

#include <cmath>
#include <sstream>
#include <string>
#include "FVector3.h"

/**
 * @struct FVector4
 * @brief Represents a 4D vector with X, Y, Z, and W components.
 *
 * Provides basic arithmetic operations, normalization, interpolation,
 * and geometric utilities such as dot products.
 */
struct FVector4
{
    float x, y, z, w;

    /** Default constructor. Initializes all components to zero. */
    constexpr FVector4() : x(0), y(0), z(0), w(0) {}

    /** Constructs a vector with the given X, Y, Z, and W values. */
    constexpr FVector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    /** Constructs a vector with all components equal to the given scalar. */
    explicit constexpr FVector4(float scalar) : x(scalar), y(scalar), z(scalar), w(scalar) {}

    /** Constructs a vector from a FVector3 with an optional W component (default 1.0f). */
    explicit constexpr FVector4(const FVector3& vec3, float wVal = 1.0f) : x(vec3.x), y(vec3.y), z(vec3.z), w(wVal) {}

    /** @return The length (magnitude) of the vector. */
    [[nodiscard]] float Length() const { return std::sqrt(x*x + y*y + z*z + w*w); }

    /**
     * @brief Returns a normalized copy of the vector.
     * @return A unit-length vector pointing in the same direction.
     */
    [[nodiscard]] FVector4 Normalized() const
    {
        float len = Length();
        if (len == 0.0f) return FVector4(0.0f);
        float inv = 1.0f / len;
        return { x * inv, y * inv, z * inv, w * inv };
    }

    /**
     * @brief Computes the dot product with another vector.
     * @param other The vector to dot against.
     * @return The scalar dot product value.
     */
    [[nodiscard]] float Dot(const FVector4& other) const { return x*other.x + y*other.y + z*other.z + w*other.w; }

    /**
     * @brief Computes the distance between this vector and another.
     * @param other The target vector.
     * @return The scalar distance between the two points.
     */
    [[nodiscard]] float Distance(const FVector4& other) const
    {
        float dx = x - other.x;
        float dy = y - other.y;
        float dz = z - other.z;
        float dw = w - other.w;
        return std::sqrt(dx*dx + dy*dy + dz*dz + dw*dw);
    }

    /**
     * @brief Linearly interpolates between this vector and another.
     * @param b The target vector.
     * @param alpha Interpolation factor (0.0 to 1.0).
     * @return The interpolated vector.
     */
    [[nodiscard]] FVector4 Lerp(const FVector4& b, float alpha) const
    {
        return {
            x * (1.0f - alpha) + b.x * alpha,
            y * (1.0f - alpha) + b.y * alpha,
            z * (1.0f - alpha) + b.z * alpha,
            w * (1.0f - alpha) + b.w * alpha
        };
    }

    /** @name Arithmetic Operators */
    ///@{

    constexpr FVector4 operator+(const FVector4& other) const { return { x + other.x, y + other.y, z + other.z, w + other.w }; }
    constexpr FVector4 operator-(const FVector4& other) const { return { x - other.x, y - other.y, z - other.z, w - other.w }; }

    FVector4& operator+=(const FVector4& other)
    {
        x += other.x; y += other.y; z += other.z; w += other.w;
        return *this;
    }

    FVector4& operator-=(const FVector4& other)
    {
        x -= other.x; y -= other.y; z -= other.z; w -= other.w;
        return *this;
    }

    /** Component-wise multiplication with another vector */
    constexpr FVector4 operator*(const FVector4& other) const
    {
        return { x * other.x, y * other.y, z * other.z, w * other.w };
    }

    /** Component-wise multiplication assignment */
    FVector4& operator*=(const FVector4& other)
    {
        x *= other.x; y *= other.y; z *= other.z; w *= other.w;
        return *this;
    }

    /** Component-wise division with another vector */
    constexpr FVector4 operator/(const FVector4& other) const
    {
        return { x / other.x, y / other.y, z / other.z, w / other.w };
    }

    /** Component-wise division assignment */
    FVector4& operator/=(const FVector4& other)
    {
        x /= other.x; y /= other.y; z /= other.z; w /= other.w;
        return *this;
    }


    constexpr FVector4 operator*(float scalar) const { return { x * scalar, y * scalar, z * scalar, w * scalar }; }
    constexpr FVector4 operator/(float scalar) const { return { x / scalar, y / scalar, z / scalar, w / scalar }; }

    FVector4& operator*=(float scalar)
    {
        x *= scalar; y *= scalar; z *= scalar; w *= scalar;
        return *this;
    }

    FVector4& operator/=(float scalar)
    {
        x /= scalar; y /= scalar; z /= scalar; w /= scalar;
        return *this;
    }

    constexpr FVector4 operator-() const { return { -x, -y, -z, -w }; }

    ///@}

    /** @name Comparison Operators */
    ///@{

    bool operator==(const FVector4& other) const
    {
        const float Epsilon = 1e-6f;
        return (std::fabs(x - other.x) < Epsilon) &&
               (std::fabs(y - other.y) < Epsilon) &&
               (std::fabs(z - other.z) < Epsilon) &&
               (std::fabs(w - other.w) < Epsilon);
    }

    bool operator!=(const FVector4& other) const { return !(*this == other); }

    ///@}

    /**
     * @brief Allows scalar multiplication from the left-hand side.
     * @param scalar The scalar value.
     * @param vec The vector to scale.
     * @return The scaled vector.
     */
    friend constexpr FVector4 operator*(float scalar, const FVector4& vec)
    {
        return { vec.x * scalar, vec.y * scalar, vec.z * scalar, vec.w * scalar };
    }

    /**
     * @brief Converts the vector to a string (formatted as "x y z w").
     * @return The string representation of the vector.
     */
    [[nodiscard]] std::string ToString() const
    {
        std::ostringstream ss;
        ss << x << " " << y << " " << z << " " << w;
        return ss.str();
    }
};

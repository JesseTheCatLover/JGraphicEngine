// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.
#pragma once

#include <sstream>
#include <string>
#include <cmath>

struct FRotator;
struct FEuler;

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
    constexpr FVector3(float x, float y, float z) : x(x), y(y), z(z) {}

    /** Constructs a vector with all components equal to the given scalar. */
    explicit constexpr FVector3(float scalar) : x(scalar), y(scalar), z(scalar) {}

    /** @return The length (magnitude) of the vector. */
    [[nodiscard]] float Length() const { return std::sqrt(x * x + y * y + z * z); }

    /**
     * @return The squared magnitude of the vector.
     *
     * Computes (x² + y² + z²) without taking the square root.
     * This is useful for fast comparisons where exact distance
     * is not required (e.g., raycasting, collision checks).
     *
     * @note Prefer this over Length() when you only need to compare
     *       distances, because it avoids the expensive sqrt().
     */
    [[nodiscard]] float LengthSquared() const
    {
        return x * x + y * y + z * z;
    }

    static FVector3 Up() { return FVector3(0.0f, 0.0f, 1.0f); }
    static FVector3 Down() { return FVector3(0.0f, 0.0f, -1.0f); }
    static FVector3 Right() { return FVector3(0.0f, 1.0f, 0.0f); }
    static FVector3 Left() { return FVector3(0.0f, -1.0f, 0.0f); }
    static FVector3 Forward() { return FVector3(1.0f, 0.0f, 0.0f); }
    static FVector3 Backward() { return FVector3(-1.0f, 0.0f, 0.0f); }

    /** @return A normalized copy of the vector. */
    [[nodiscard]] FVector3 Normalized() const
    {
        float len = Length();
        if (len == 0.0f) return FVector3(0.0f);
        float inv = 1.0f / len;
        return { x * inv, y * inv, z * inv };
    }

    /** Dot product with another vector */
    [[nodiscard]] float Dot(const FVector3& vec) const { return x * vec.x + y * vec.y + z * vec.z; }

    /** Cross product with another vector */
    [[nodiscard]] FVector3 Cross(const FVector3& vec) const
    {
        return {
            y * vec.z - z * vec.y,
            z * vec.x - x * vec.z,
            x * vec.y - y * vec.x
        };
    }

    /** Distance to another vector */
    [[nodiscard]] float Distance(const FVector3& vec) const
    {
        float dx = x - vec.x;
        float dy = y - vec.y;
        float dz = z - vec.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    /** Linearly interpolate toward another vector */
    [[nodiscard]] FVector3 Lerp(const FVector3& vec, float alpha) const
    {
        return {
            x * (1.0f - alpha) + vec.x * alpha,
            y * (1.0f - alpha) + vec.y * alpha,
            z * (1.0f - alpha) + vec.z * alpha
        };
    }

    /** @name Arithmetic Operators */
    ///@{
    constexpr FVector3 operator+(const FVector3& vec) const { return { x + vec.x, y + vec.y, z + vec.z }; }
    constexpr FVector3 operator-(const FVector3& vec) const { return { x - vec.x, y - vec.y, z - vec.z }; }

    FVector3& operator+=(const FVector3& vec)
    {
        x += vec.x; y += vec.y; z += vec.z;
        return *this;
    }

    FVector3& operator-=(const FVector3& vec)
    {
        x -= vec.x; y -= vec.y; z -= vec.z;
        return *this;
    }

    constexpr FVector3 operator*(float scalar) const { return { x * scalar, y * scalar, z * scalar }; }
    constexpr FVector3 operator/(float scalar) const { return { x / scalar, y / scalar, z / scalar }; }

    FVector3& operator*=(float scalar)
    {
        x *= scalar; y *= scalar; z *= scalar;
        return *this;
    }

    FVector3& operator/=(float scalar)
    {
        x /= scalar; y /= scalar; z /= scalar;
        return *this;
    }

    constexpr FVector3 operator-() const { return { -x, -y, -z }; }

    constexpr FVector3 operator*(const FVector3& vec) const { return { x * vec.x, y * vec.y, z * vec.z }; }
    FVector3& operator*=(const FVector3& vec)
    {
        x *= vec.x; y *= vec.y; z *= vec.z;
        return *this;
    }

    constexpr FVector3 operator/(const FVector3& vec) const { return { x / vec.x, y / vec.y, z / vec.z }; }
    FVector3& operator/=(const FVector3& vec)
    {
        x /= vec.x; y /= vec.y; z /= vec.z;
        return *this;
    }

    /** Comparison */
    bool operator==(const FVector3& vec) const
    {
        const float epsilon = 1e-6f;
        return std::fabs(x - vec.x) < epsilon &&
               std::fabs(y - vec.y) < epsilon &&
               std::fabs(z - vec.z) < epsilon;
    }

    bool operator!=(const FVector3& vec) const { return !(*this == vec); }

    /** Left-hand scalar multiplication */
    friend constexpr FVector3 operator*(float scalar, const FVector3& vec)
    {
        return { vec.x * scalar, vec.y * scalar, vec.z * scalar };
    }

    const float* ToFloat3()
    {
        return &this->x;
    }

    [[nodiscard]] FRotator ToRotator() const;

    [[nodiscard]] FEuler ToEuler() const;

    /** String representation */
    [[nodiscard]] std::string ToString() const
    {
        std::ostringstream ss;
        ss << x << " " << y << " " << z;
        return ss.str();
    }
};

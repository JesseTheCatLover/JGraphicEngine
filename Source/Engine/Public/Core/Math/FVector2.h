// Copyright 2025 JesseTheCatLover. All Rights Reserved.
#pragma once

#include <sstream>
#include <string>
#include <cmath>

/**
 * @struct FVector2
 * @brief Represents a 2D vector with X and Y components.
 *
 * Provides basic arithmetic operations, normalization, interpolation,
 * and geometric utilities such as dot products.
 */
struct FVector2
{
    float x, y;

    /** Default constructor. Initializes both components to zero. */
    constexpr FVector2() : x(0), y(0) {}

    /** Constructs a vector with the given X and Y values. */
    constexpr FVector2(float x, float y) : x(x), y(y) {}

    /** Constructs a vector with both components equal to the given scalar. */
    explicit constexpr FVector2(float scalar) : x(scalar), y(scalar) {}

    /** @return The length (magnitude) of the vector. */
    [[nodiscard]] float Length() const { return std::sqrt(x * x + y * y); }

    /**
     * @brief Returns a normalized copy of the vector.
     * @return A unit-length vector pointing in the same direction.
     */
    [[nodiscard]] FVector2 Normalized() const
    {
        float len = Length();
        if (len == 0.0f) return FVector2(0.0f);
        float inv = 1.0f / len;
        return { x * inv, y * inv };
    }

    /**
     * @brief Computes the dot product with another vector.
     * @param vec The vector to dot against.
     * @return The scalar dot product value.
     */
    [[nodiscard]] float Dot(const FVector2& vec) const { return x * vec.x + y * vec.y; }

    /**
     * @brief Computes the distance between this vector and another.
     * @param vec The target vector.
     * @return The scalar distance between the two points.
     */
    [[nodiscard]] float Distance(const FVector2& vec) const
    {
        float dx = x - vec.x;
        float dy = y - vec.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    /**
     * @brief Linearly interpolates between this vector and another.
     * @param vec The target vector.
     * @param alpha Interpolation factor (0.0 to 1.0).
     * @return The interpolated vector.
     */
    [[nodiscard]] FVector2 Lerp(const FVector2& vec, float alpha) const
    {
        return {
            x * (1.0f - alpha) + vec.x * alpha,
            y * (1.0f - alpha) + vec.y * alpha
        };
    }

    /** @name Arithmetic Operators */
    ///@{

    constexpr FVector2 operator+(const FVector2& vec) const { return { x + vec.x, y + vec.y }; }
    constexpr FVector2 operator-(const FVector2& vec) const { return { x - vec.x, y - vec.y }; }

    FVector2& operator+=(const FVector2& vec)
    {
        x += vec.x;
        y += vec.y;
        return *this;
    }

    FVector2& operator-=(const FVector2& vec)
    {
        x -= vec.x;
        y -= vec.y;
        return *this;
    }

    constexpr FVector2 operator*(const FVector2& vec) const { return { x * vec.x, y * vec.y }; }
    constexpr FVector2 operator/(const FVector2& vec) const { return { x / vec.x, y / vec.y }; }

    FVector2& operator*=(const FVector2& vec)
    {
        x *= vec.x;
        y *= vec.y;
        return *this;
    }

    FVector2& operator/=(const FVector2& vec)
    {
        x /= vec.x;
        y /= vec.y;
        return *this;
    }

    constexpr FVector2 operator*(float scalar) const { return { x * scalar, y * scalar }; }
    constexpr FVector2 operator/(float scalar) const { return { x / scalar, y / scalar }; }

    FVector2& operator*=(float scalar)
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    FVector2& operator/=(float scalar)
    {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    constexpr FVector2 operator-() const { return { -x, -y }; }

    ///@}

    /** @name Comparison Operators */
    ///@{

    bool operator==(const FVector2& vec) const
    {
        const float epsilon = 1e-6f;
        return (std::fabs(x - vec.x) < epsilon) &&
               (std::fabs(y - vec.y) < epsilon);
    }

    bool operator!=(const FVector2& vec) const { return !(*this == vec); }

    ///@}

    /**
     * @brief Allows scalar multiplication from the left-hand side.
     * @param scalar The scalar value.
     * @param vec The vector to scale.
     * @return The scaled vector.
     */
    friend constexpr FVector2 operator*(float scalar, const FVector2& vec)
    {
        return { vec.x * scalar, vec.y * scalar };
    }

    /**
     * @brief Converts the vector to a string (formatted as "x y").
     * @return The string representation of the vector.
     */
    [[nodiscard]] std::string ToString() const
    {
        std::ostringstream ss;
        ss << x << " " << y;
        return ss.str();
    }
};

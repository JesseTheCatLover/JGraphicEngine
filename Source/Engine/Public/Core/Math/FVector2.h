// Copyright 2025 JesseTheCatLover. All Rights Reserved.
#pragma once

#include <sstream>
#include <string>

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
    constexpr FVector2(float X, float Y) : x(X), y(Y) {}

    /** Constructs a vector with both components equal to the given scalar. */
    explicit constexpr FVector2(float Scalar) : x(Scalar), y(Scalar) {}

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
     * @param Other The vector to dot against.
     * @return The scalar dot product value.
     */
    [[nodiscard]] float Dot(const FVector2& Other) const { return x * Other.x + y * Other.y; }

    /**
     * @brief Computes the distance between this vector and another.
     * @param Other The target vector.
     * @return The scalar distance between the two points.
     */
    [[nodiscard]] float Distance(const FVector2& Other) const
    {
        float dx = x - Other.x;
        float dy = y - Other.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    /**
     * @brief Linearly interpolates between this vector and another.
     * @param B The target vector.
     * @param Alpha Interpolation factor (0.0 to 1.0).
     * @return The interpolated vector.
     */
    [[nodiscard]] FVector2 Lerp(const FVector2& B, float Alpha) const
    {
        return {
            x * (1.0f - Alpha) + B.x * Alpha,
            y * (1.0f - Alpha) + B.y * Alpha
        };
    }

    /** @name Arithmetic Operators */
    ///@{

    constexpr FVector2 operator+(const FVector2& Other) const { return { x + Other.x, y + Other.y }; }
    constexpr FVector2 operator-(const FVector2& Other) const { return { x - Other.x, y - Other.y }; }

    FVector2& operator+=(const FVector2& Other)
    {
        x += Other.x;
        y += Other.y;
        return *this;
    }

    FVector2& operator-=(const FVector2& Other)
    {
        x -= Other.x;
        y -= Other.y;
        return *this;
    }

    constexpr FVector2 operator*(const FVector2 &Other) const { return {x * Other.x, y * Other.y}; }
    constexpr FVector2 operator/(const FVector2 &Other) const { return {x / Other.x, y / Other.y}; }

    FVector2& operator*=(const FVector2& Other)
    {
        x *= Other.x;
        y *= Other.y;
        return *this;
    }

    FVector2& operator/=(const FVector2& Other)
    {
        x /= Other.x;
        y /= Other.y;
        return *this;
    }

    constexpr FVector2 operator*(float Scalar) const { return { x * Scalar, y * Scalar }; }
    constexpr FVector2 operator/(float Scalar) const { return { x / Scalar, y / Scalar }; }

    FVector2& operator*=(float Scalar)
    {
        x *= Scalar;
        y *= Scalar;
        return *this;
    }

    FVector2& operator/=(float Scalar)
    {
        x /= Scalar;
        y /= Scalar;
        return *this;
    }

    constexpr FVector2 operator-() const { return { -x, -y }; }

    ///@}

    /** @name Comparison Operators */
    ///@{

    bool operator==(const FVector2& Other) const
    {
        const float Epsilon = 1e-6f;
        return (std::fabs(x - Other.x) < Epsilon) &&
               (std::fabs(y - Other.y) < Epsilon);
    }

    bool operator!=(const FVector2& Other) const { return !(*this == Other); }

    ///@}

    /**
     * @brief Allows scalar multiplication from the left-hand side.
     * @param Scalar The scalar value.
     * @param Vec The vector to scale.
     * @return The scaled vector.
     */
    friend constexpr FVector2 operator*(float Scalar, const FVector2& Vec)
    {
        return { Vec.x * Scalar, Vec.y * Scalar };
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

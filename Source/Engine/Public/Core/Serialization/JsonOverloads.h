//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "nlohmann/json.hpp"

#include "Core/Math/FVector2.h"
#include "Core/Math/FVector3.h"
#include "Core/Math/FVector4.h"
#include "Core/Math/FQuat.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FEuler.h"
#include "Core/Math/FTransform.h"

/**
 * @file JsonOverloads.h
 * @brief Defines serialization overloads for engine math types.
 *
 * These `to_json()` and `from_json()` functions allow nlohmann::json
 * to automatically serialize and deserialize JEngine’s core math types.
 *
 * They enable generic code such as:
 * @code
 * json j;
 * j["Position"] = FVector3(1, 2, 3);
 * FVector3 pos = j["Position"].get<FVector3>();
 * @endcode
 */

// =========================================================
// FVector2
// =========================================================

inline void to_json(nlohmann::json& j, const FVector2& v)
{
    j = nlohmann::json{{"x", v.x}, {"y", v.y}};
}

inline void from_json(const nlohmann::json& j, FVector2& v)
{
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
}

// =========================================================
// FVector3
// =========================================================

inline void to_json(nlohmann::json& j, const FVector3& v)
{
    j = nlohmann::json{{"x", v.x}, {"y", v.y}, {"z", v.z}};
}

inline void from_json(const nlohmann::json& j, FVector3& v)
{
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
    j.at("z").get_to(v.z);
}

// =========================================================
// FVector4
// =========================================================

inline void to_json(nlohmann::json& j, const FVector4& v)
{
    j = nlohmann::json{{"x", v.x}, {"y", v.y}, {"z", v.z}, {"w", v.w}};
}

inline void from_json(const nlohmann::json& j, FVector4& v)
{
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
    j.at("z").get_to(v.z);
    j.at("w").get_to(v.w);
}

// =========================================================
// FQuat
// =========================================================

inline void to_json(nlohmann::json& j, const FQuat& q)
{
    j = nlohmann::json{
        {"x", q.x()},
        {"y", q.y()},
        {"z", q.z()},
        {"w", q.w()}
    };
}

inline void from_json(const nlohmann::json& j, FQuat& q)
{
    float x, y, z, w;
    j.at("x").get_to(x);
    j.at("y").get_to(y);
    j.at("z").get_to(z);
    j.at("w").get_to(w);
    q = FQuat(x, y, z, w);
}

// =========================================================
// FRotator (degrees)
// =========================================================

inline void to_json(nlohmann::json& j, const FRotator& r)
{
    j = nlohmann::json{
        {"pitch", r.Pitch},
        {"yaw",   r.Yaw},
        {"roll",  r.Roll}
    };
}

inline void from_json(const nlohmann::json& j, FRotator& r)
{
    j.at("pitch").get_to(r.Pitch);
    j.at("yaw").get_to(r.Yaw);
    j.at("roll").get_to(r.Roll);
}

// =========================================================
// FEuler (radians)
// =========================================================

inline void to_json(nlohmann::json& j, const FEuler& e)
{
    j = nlohmann::json{
        {"pitch", e.Pitch},
        {"yaw",   e.Yaw},
        {"roll",  e.Roll}
    };
}

inline void from_json(const nlohmann::json& j, FEuler& e)
{
    j.at("pitch").get_to(e.Pitch);
    j.at("yaw").get_to(e.Yaw);
    j.at("roll").get_to(e.Roll);
}

// =========================================================
// FTransform
// =========================================================

inline void to_json(nlohmann::json& j, const FTransform& t)
{
    j = nlohmann::json{
        {"position", t.GetPosition()},
        {"rotation", t.GetRotation()},
        {"scale",    t.GetScale()}
    };
}

inline void from_json(const nlohmann::json& j, FTransform& t)
{
    t.SetPosition(j.at("position").get<FVector3>());
    t.SetRotation(j.at("rotation").get<FQuat>());
    t.SetScale(j.at("scale").get<FVector3>());
}

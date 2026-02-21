//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

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
 * (and nlohmann::ordered_json) to automatically serialize and deserialize
 * JEngine’s core math types.
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

template <typename BasicJsonType>
inline void to_json(BasicJsonType& j, const FVector2& v)
{
    j = BasicJsonType{
        {"x", v.x},
        {"y", v.y}
    };
}

template <typename BasicJsonType>
inline void from_json(const BasicJsonType& j, FVector2& v)
{
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
}

// =========================================================
// FVector3
// =========================================================

template <typename BasicJsonType>
inline void to_json(BasicJsonType& j, const FVector3& v)
{
    j = BasicJsonType{
        {"x", v.x},
        {"y", v.y},
        {"z", v.z}
    };
}

template <typename BasicJsonType>
inline void from_json(const BasicJsonType& j, FVector3& v)
{
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
    j.at("z").get_to(v.z);
}

// =========================================================
// FVector4
// =========================================================

template <typename BasicJsonType>
inline void to_json(BasicJsonType& j, const FVector4& v)
{
    j = BasicJsonType{
        {"x", v.x},
        {"y", v.y},
        {"z", v.z},
        {"w", v.w}
    };
}

template <typename BasicJsonType>
inline void from_json(const BasicJsonType& j, FVector4& v)
{
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
    j.at("z").get_to(v.z);
    j.at("w").get_to(v.w);
}

// =========================================================
// FQuat
// =========================================================

template <typename BasicJsonType>
inline void to_json(BasicJsonType& j, const FQuat& q)
{
    j = BasicJsonType{
        {"x", q.x()},
        {"y", q.y()},
        {"z", q.z()},
        {"w", q.w()}
    };
}

template <typename BasicJsonType>
inline void from_json(const BasicJsonType& j, FQuat& q)
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

template <typename BasicJsonType>
inline void to_json(BasicJsonType& j, const FRotator& r)
{
    j = BasicJsonType{
        {"pitch", r.pitch},
        {"yaw",   r.yaw},
        {"roll",  r.roll}
    };
}

template <typename BasicJsonType>
inline void from_json(const BasicJsonType& j, FRotator& r)
{
    j.at("pitch").get_to(r.pitch);
    j.at("yaw").get_to(r.yaw);
    j.at("roll").get_to(r.roll);
}

// =========================================================
// FEuler (radians)
// =========================================================

template <typename BasicJsonType>
inline void to_json(BasicJsonType& j, const FEuler& e)
{
    j = BasicJsonType{
        {"pitch", e.Pitch},
        {"yaw",   e.Yaw},
        {"roll",  e.Roll}
    };
}

template <typename BasicJsonType>
inline void from_json(const BasicJsonType& j, FEuler& e)
{
    j.at("pitch").get_to(e.Pitch);
    j.at("yaw").get_to(e.Yaw);
    j.at("roll").get_to(e.Roll);
}

// =========================================================
// FTransform
// =========================================================

template <typename BasicJsonType>
inline void to_json(BasicJsonType& j, const FTransform& t)
{
    j = BasicJsonType{
        {"position", t.GetPosition()},
        {"rotation", t.GetRotation()},
        {"scale",    t.GetScale()}
    };
}

template <typename BasicJsonType>
inline void from_json(const BasicJsonType& j, FTransform& t)
{
    t.SetPosition(j.at("position").template get<FVector3>());
    t.SetRotation(j.at("rotation").template get<FQuat>());
    t.SetScale   (j.at("scale").template get<FVector3>());
}

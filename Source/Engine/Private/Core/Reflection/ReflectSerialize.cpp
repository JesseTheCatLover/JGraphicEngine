//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Reflection/ReflectSerialize.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "Core/JCoreObject.h"
#include "Core/Reflection/RETypeRegistry.h"

namespace
{
    using REUpcastFn = const void*(*)(const void *thisAsDerived); // returns base subobject ptr


    // ---------------- Small string helpers ----------------

    static std::string StripSpaces(std::string s)
    {
        s.erase(std::remove_if(s.begin(), s.end(),
                               [](unsigned char c) { return std::isspace(c) != 0; }), s.end());
        return s;
    }

    static bool TypeNameIsPointer(const std::string &tn)
    {
        return tn.find('*') != std::string::npos;
    }

    static std::string StripPointerStars(std::string s)
    {
        s.erase(std::remove(s.begin(), s.end(), '*'), s.end());
        return s;
    }

    // ---------------- Enum helpers ----------------

    static size_t EnumUnderlyingSizeBytes(const REEnum &e)
    {
        std::string u = StripSpaces(e.underlyingType);

        if (u.empty()) return sizeof(int);

        if (u == "uint8_t" || u == "unsignedchar" || u == "unsigned__int8") return 1;
        if (u == "int8_t" || u == "signedchar" || u == "__int8") return 1;
        if (u == "uint16_t" || u == "unsignedshort" || u == "unsigned__int16") return 2;
        if (u == "int16_t" || u == "short" || u == "__int16") return 2;
        if (u == "uint32_t" || u == "unsignedint" || u == "unsigned__int32") return 4;
        if (u == "int32_t" || u == "int" || u == "__int32") return 4;
        if (u == "uint64_t" || u == "unsignedlonglong" || u == "unsigned__int64") return 8;
        if (u == "int64_t" || u == "longlong" || u == "__int64") return 8;

        return sizeof(int);
    }

    static const char *EnumFindNameByValue(const REEnum &e, int64_t v)
    {
        (void) e;
        (void) v;
        return nullptr;
    }

    static bool EnumTryParseByName(const REEnum &e, const std::string &name, int64_t &outValue)
    {
        for (const auto &ev: e.values)
        {
            if (ev.name == name)
            {
                if (!ev.valueExpr.empty())
                {
                    char *end = nullptr;
                    long long parsed = std::strtoll(ev.valueExpr.c_str(), &end, 10);
                    if (end && *end == '\0')
                    {
                        outValue = (int64_t) parsed;
                        return true;
                    }
                }
                return false;
            }
        }
        return false;
    }

    // ---------------- Value IO (typeName-based) ----------------

    static bool WriteValueByTypeName(JsonWriter &writer, const char *name, const void *fieldPtr,
                                     const std::string &typeName)
    {
        if (typeName == "int" || typeName == "int32" || typeName == "int32_t")
        {
            writer.Write(name, *reinterpret_cast<const int *>(fieldPtr));
            return true;
        }
        if (typeName == "size_t" || typeName == "std::size_t")
        {
            writer.Write(name, *reinterpret_cast<const size_t *>(fieldPtr));
            return true;
        }
        if (typeName == "float")
        {
            writer.Write(name, *reinterpret_cast<const float *>(fieldPtr));
            return true;
        }
        if (typeName == "double")
        {
            writer.Write(name, *reinterpret_cast<const double *>(fieldPtr));
            return true;
        }
        if (typeName == "bool")
        {
            writer.Write(name, *reinterpret_cast<const bool *>(fieldPtr));
            return true;
        }
        if (typeName == "std::string" || typeName == "string")
        {
            writer.Write(name, *reinterpret_cast<const std::string *>(fieldPtr));
            return true;
        }

        if (typeName == "FVector2")
        {
            writer.WriteVect2(name, *reinterpret_cast<const FVector2 *>(fieldPtr));
            return true;
        }
        if (typeName == "FVector3")
        {
            writer.WriteVect3(name, *reinterpret_cast<const FVector3 *>(fieldPtr));
            return true;
        }
        if (typeName == "FVector4")
        {
            writer.WriteVect4(name, *reinterpret_cast<const FVector4 *>(fieldPtr));
            return true;
        }
        if (typeName == "FMatrix4")
        {
            writer.WriteMatrix4(name, *reinterpret_cast<const FMatrix4 *>(fieldPtr));
            return true;
        }
        if (typeName == "FRotator")
        {
            writer.WriteRotator(name, *reinterpret_cast<const FRotator *>(fieldPtr));
            return true;
        }
        if (typeName == "FQuat")
        {
            writer.WriteQuat(name, *reinterpret_cast<const FQuat *>(fieldPtr));
            return true;
        }
        if (typeName == "FTransform")
        {
            writer.WriteTransform(name, *reinterpret_cast<const FTransform *>(fieldPtr));
            return true;
        }

        return false;
    }

    // --- NEW HELPER: Appends raw values to arrays instead of using a key ---
    static bool WriteArrayElementByTypeName(JsonWriter &writer, const void *fieldPtr, const std::string &typeName)
    {
        if (typeName == "int" || typeName == "int32" || typeName == "int32_t")
        {
            writer.WriteValue(*reinterpret_cast<const int *>(fieldPtr));
            return true;
        }
        if (typeName == "size_t" || typeName == "std::size_t")
        {
            writer.WriteValue(*reinterpret_cast<const size_t *>(fieldPtr));
            return true;
        }
        if (typeName == "float")
        {
            writer.WriteValue(*reinterpret_cast<const float *>(fieldPtr));
            return true;
        }
        if (typeName == "double")
        {
            writer.WriteValue(*reinterpret_cast<const double *>(fieldPtr));
            return true;
        }
        if (typeName == "bool")
        {
            writer.WriteValue(*reinterpret_cast<const bool *>(fieldPtr));
            return true;
        }
        if (typeName == "std::string" || typeName == "string")
        {
            writer.WriteValue(*reinterpret_cast<const std::string *>(fieldPtr));
            return true;
        }

        // Math types auto-serialize using their JsonOverloads.h to_json signatures
        if (typeName == "FVector2")
        {
            writer.WriteValue(*reinterpret_cast<const FVector2 *>(fieldPtr));
            return true;
        }
        if (typeName == "FVector3")
        {
            writer.WriteValue(*reinterpret_cast<const FVector3 *>(fieldPtr));
            return true;
        }
        if (typeName == "FVector4")
        {
            writer.WriteValue(*reinterpret_cast<const FVector4 *>(fieldPtr));
            return true;
        }
        if (typeName == "FRotator")
        {
            writer.WriteValue(*reinterpret_cast<const FRotator *>(fieldPtr));
            return true;
        }
        if (typeName == "FQuat")
        {
            writer.WriteValue(*reinterpret_cast<const FQuat *>(fieldPtr));
            return true;
        }
        if (typeName == "FTransform")
        {
            writer.WriteValue(*reinterpret_cast<const FTransform *>(fieldPtr));
            return true;
        }

        if (typeName == "FMatrix4")
        {
            const FMatrix4 &mat = *reinterpret_cast<const FMatrix4 *>(fieldPtr);
            JJson matJson = JJson::array();
            for (int r = 0; r < 4; ++r)
            {
                JJson rowJson = JJson::array();
                for (int c = 0; c < 4; ++c) rowJson.push_back(mat.GetMat4()[r][c]);
                matJson.push_back(rowJson);
            }
            writer.WriteValue(matJson);
            return true;
        }

        return false;
    }

    static bool ReadValueByTypeName(const JsonReader &reader, const char *name, void *fieldPtr,
                                    const std::string &typeName)
    {
        if (typeName == "int" || typeName == "int32" || typeName == "int32_t")
        {
            auto &ref = *reinterpret_cast<int *>(fieldPtr);
            ref = reader.Read(name, ref);
            return true;
        }
        if (typeName == "size_t" || typeName == "std::size_t")
        {
            auto &ref = *reinterpret_cast<size_t *>(fieldPtr);
            ref = reader.Read(name, ref);
            return true;
        }
        if (typeName == "float")
        {
            auto &ref = *reinterpret_cast<float *>(fieldPtr);
            ref = reader.Read(name, ref);
            return true;
        }
        if (typeName == "double")
        {
            auto &ref = *reinterpret_cast<double *>(fieldPtr);
            ref = reader.Read(name, ref);
            return true;
        }
        if (typeName == "bool")
        {
            auto &ref = *reinterpret_cast<bool *>(fieldPtr);
            ref = reader.Read(name, ref);
            return true;
        }
        if (typeName == "std::string" || typeName == "string")
        {
            auto &ref = *reinterpret_cast<std::string *>(fieldPtr);
            ref = reader.Read(name, ref);
            return true;
        }

        if (typeName == "FVector2")
        {
            auto &ref = *reinterpret_cast<FVector2 *>(fieldPtr);
            ref = reader.ReadVector2(name, ref);
            return true;
        }
        if (typeName == "FVector3")
        {
            auto &ref = *reinterpret_cast<FVector3 *>(fieldPtr);
            ref = reader.ReadVector3(name, ref);
            return true;
        }
        if (typeName == "FVector4")
        {
            auto &ref = *reinterpret_cast<FVector4 *>(fieldPtr);
            ref = reader.ReadVector4(name, ref);
            return true;
        }
        if (typeName == "FMatrix4")
        {
            auto &ref = *reinterpret_cast<FMatrix4 *>(fieldPtr);
            ref = reader.ReadMatrix4(name, ref);
            return true;
        }
        if (typeName == "FRotator")
        {
            auto &ref = *reinterpret_cast<FRotator *>(fieldPtr);
            ref = reader.ReadRotator(name, ref);
            return true;
        }
        if (typeName == "FQuat")
        {
            auto &ref = *reinterpret_cast<FQuat *>(fieldPtr);
            ref = reader.ReadQuat(name, ref);
            return true;
        }
        if (typeName == "FTransform")
        {
            auto &ref = *reinterpret_cast<FTransform *>(fieldPtr);
            ref = reader.ReadTransform(name, ref);
            return true;
        }

        return false;
    }

    static bool ReadValueFromJson(const JJson &elem, void *fieldPtr, const std::string &typeName)
    {
        try
        {
            if (typeName == "int" || typeName == "int32" || typeName == "int32_t")
            {
                *reinterpret_cast<int *>(fieldPtr) = elem.get<int>();
                return true;
            }
            if (typeName == "size_t" || typeName == "std::size_t")
            {
                *reinterpret_cast<size_t *>(fieldPtr) = elem.get<size_t>();
                return true;
            }
            if (typeName == "float")
            {
                *reinterpret_cast<float *>(fieldPtr) = elem.get<float>();
                return true;
            }
            if (typeName == "double")
            {
                *reinterpret_cast<double *>(fieldPtr) = elem.get<double>();
                return true;
            }
            if (typeName == "bool")
            {
                *reinterpret_cast<bool *>(fieldPtr) = elem.get<bool>();
                return true;
            }
            if (typeName == "std::string" || typeName == "string")
            {
                *reinterpret_cast<std::string *>(fieldPtr) = elem.get<std::string>();
                return true;
            }

            if (typeName == "FVector2")
            {
                *reinterpret_cast<FVector2 *>(fieldPtr) = elem.get<FVector2>();
                return true;
            }
            if (typeName == "FVector3")
            {
                *reinterpret_cast<FVector3 *>(fieldPtr) = elem.get<FVector3>();
                return true;
            }
            if (typeName == "FVector4")
            {
                *reinterpret_cast<FVector4 *>(fieldPtr) = elem.get<FVector4>();
                return true;
            }
            if (typeName == "FRotator")
            {
                *reinterpret_cast<FRotator *>(fieldPtr) = elem.get<FRotator>();
                return true;
            }
            if (typeName == "FQuat")
            {
                *reinterpret_cast<FQuat *>(fieldPtr) = elem.get<FQuat>();
                return true;
            }
            if (typeName == "FTransform")
            {
                *reinterpret_cast<FTransform *>(fieldPtr) = elem.get<FTransform>();
                return true;
            }

            if (typeName == "FMatrix4")
            {
                FMatrix4 result;
                if (elem.is_array() && elem.size() == 4)
                {
                    for (int r = 0; r < 4; ++r)
                    {
                        if (elem[r].is_array() && elem[r].size() == 4)
                        {
                            for (int c = 0; c < 4; ++c) result.GetMat4()[r][c] = elem[r][c].get<float>();
                        }
                    }
                }
                *reinterpret_cast<FMatrix4 *>(fieldPtr) = result;
                return true;
            }
        } catch (...)
        {
            return false;
        }
        return false;
    }

    static bool EqualsValueByTypeName(const void *a, const void *b, const std::string &typeName)
    {
        if (typeName == "int" || typeName == "int32" || typeName == "int32_t") return
                *reinterpret_cast<const int *>(a) == *reinterpret_cast<const int *>(b);
        if (typeName == "float") return *reinterpret_cast<const float *>(a) == *reinterpret_cast<const float *>(b);
        if (typeName == "double") return *reinterpret_cast<const double *>(a) == *reinterpret_cast<const double *>(b);
        if (typeName == "bool") return *reinterpret_cast<const bool *>(a) == *reinterpret_cast<const bool *>(b);
        if (typeName == "std::string" || typeName == "string") return
                *reinterpret_cast<const std::string *>(a) == *reinterpret_cast<const std::string *>(b);

        if (typeName == "FVector2") return *reinterpret_cast<const FVector2 *>(a) == *reinterpret_cast<const FVector2 *>
                                           (b);
        if (typeName == "FVector3") return *reinterpret_cast<const FVector3 *>(a) == *reinterpret_cast<const FVector3 *>
                                           (b);
        if (typeName == "FVector4") return *reinterpret_cast<const FVector4 *>(a) == *reinterpret_cast<const FVector4 *>
                                           (b);
        if (typeName == "FRotator") return *reinterpret_cast<const FRotator *>(a) == *reinterpret_cast<const FRotator *>
                                           (b);
        if (typeName == "FQuat") return *reinterpret_cast<const FQuat *>(a) == *reinterpret_cast<const FQuat *>(b);
        if (typeName == "FTransform") return *reinterpret_cast<const FTransform *>(a) == *reinterpret_cast<const
                                                 FTransform *>(b);

        if (typeName == "FMatrix4")
        {
            const auto &ma = *reinterpret_cast<const FMatrix4 *>(a);
            const auto &mb = *reinterpret_cast<const FMatrix4 *>(b);
            for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c) if (ma.GetMat4()[r][c] != mb.GetMat4()[r][c]) return
                    false;
            return true;
        }

        return false; // unsupported -> treat as different (forces save)
    }

    static bool TryGetValueByTypeName(const void *fieldPtr, const std::string &typeName, REVariant &out)
    {
        if (typeName == "int" || typeName == "int32" || typeName == "int32_t")
        {
            out.tag = REValueTag::Int;
            out.i32 = *reinterpret_cast<const int *>(fieldPtr);
            return true;
        }
        if (typeName == "float")
        {
            out.tag = REValueTag::Float;
            out.f32 = *reinterpret_cast<const float *>(fieldPtr);
            return true;
        }
        if (typeName == "double")
        {
            out.tag = REValueTag::Double;
            out.f64 = *reinterpret_cast<const double *>(fieldPtr);
            return true;
        }
        if (typeName == "bool")
        {
            out.tag = REValueTag::Bool;
            out.b = *reinterpret_cast<const bool *>(fieldPtr);
            return true;
        }
        if (typeName == "std::string" || typeName == "string")
        {
            out.tag = REValueTag::String;
            out.s = *reinterpret_cast<const std::string *>(fieldPtr);
            return true;
        }
        if (typeName == "FVector2")
        {
            out.tag = REValueTag::Vec2;
            out.v2 = *reinterpret_cast<const FVector2 *>(fieldPtr);
            return true;
        }
        if (typeName == "FVector3")
        {
            out.tag = REValueTag::Vec3;
            out.v3 = *reinterpret_cast<const FVector3 *>(fieldPtr);
            return true;
        }
        if (typeName == "FVector4")
        {
            out.tag = REValueTag::Vec4;
            out.v4 = *reinterpret_cast<const FVector4 *>(fieldPtr);
            return true;
        }
        if (typeName == "FQuat")
        {
            out.tag = REValueTag::Quat;
            out.q = *reinterpret_cast<const FQuat *>(fieldPtr);
            return true;
        }
        if (typeName == "FTransform")
        {
            out.tag = REValueTag::Transform;
            out.t = *reinterpret_cast<const FTransform *>(fieldPtr);
            return true;
        }

        return false;
    }

    static bool TrySetValueByTypeName(void *fieldPtr, const std::string &typeName, const REVariant &in)
    {
        if (typeName == "int" || typeName == "int32" || typeName == "int32_t")
        {
            if (in.tag != REValueTag::Int) return false;
            *reinterpret_cast<int *>(fieldPtr) = in.i32;
            return true;
        }
        if (typeName == "float")
        {
            if (in.tag != REValueTag::Float) return false;
            *reinterpret_cast<float *>(fieldPtr) = in.f32;
            return true;
        }
        if (typeName == "double")
        {
            if (in.tag != REValueTag::Double) return false;
            *reinterpret_cast<double *>(fieldPtr) = in.f64;
            return true;
        }
        if (typeName == "bool")
        {
            if (in.tag != REValueTag::Bool) return false;
            *reinterpret_cast<bool *>(fieldPtr) = in.b;
            return true;
        }
        if (typeName == "std::string" || typeName == "string")
        {
            if (in.tag != REValueTag::String) return false;
            *reinterpret_cast<std::string *>(fieldPtr) = in.s;
            return true;
        }
        if (typeName == "FVector2")
        {
            if (in.tag != REValueTag::Vec2) return false;
            *reinterpret_cast<FVector2 *>(fieldPtr) = in.v2;
            return true;
        }
        if (typeName == "FVector3")
        {
            if (in.tag != REValueTag::Vec3) return false;
            *reinterpret_cast<FVector3 *>(fieldPtr) = in.v3;
            return true;
        }
        if (typeName == "FVector4")
        {
            if (in.tag != REValueTag::Vec4) return false;
            *reinterpret_cast<FVector4 *>(fieldPtr) = in.v4;
            return true;
        }
        if (typeName == "FQuat")
        {
            if (in.tag != REValueTag::Quat) return false;
            *reinterpret_cast<FQuat *>(fieldPtr) = in.q;
            return true;
        }
        if (typeName == "FTransform")
        {
            if (in.tag != REValueTag::Transform) return false;
            *reinterpret_cast<FTransform *>(fieldPtr) = in.t;
            return true;
        }

        return false;
    }

    static const void* GetFieldPtrConst(const REProperty& prop, const void* basePtr)
    {
        return prop.getConstPtr ? prop.getConstPtr(basePtr) : nullptr;
    }

    static void* GetFieldPtr(const REProperty& prop, void* basePtr)
    {
        return prop.getPtr ? prop.getPtr(basePtr) : nullptr;
    }
}

// ---------------- Resolver ----------------

static ReflectSerialize::ResolveObjectByUUIDFn g_ObjectResolver = nullptr;

void ReflectSerialize::SetObjectResolver(ResolveObjectByUUIDFn fn)
{
    g_ObjectResolver = fn;
}

bool ReflectSerialize::TryGet(const JCoreObject& obj, const REProperty& prop, REVariant& out)
{
    const void* basePtr = &obj;
    const void* fieldPtr = prop.getConstPtr ? prop.getConstPtr(basePtr) : nullptr;
    if (!fieldPtr) return false;

    switch (prop.kind)
    {
        case REPropKind::Value:
            return TryGetValueByTypeName(fieldPtr, prop.typeName, out);

        case REPropKind::Enum:
        {
            if (!prop.enumType) return false;
            const size_t sz = EnumUnderlyingSizeBytes(*prop.enumType);
            int64_t v = 0;
            std::memcpy(&v, fieldPtr, std::min(sz, sizeof(v)));
            out.tag = REValueTag::EnumInt64;
            out.i64 = v;
            return true;
        }

        case REPropKind::ObjectPtr:
        {
            auto* o = *reinterpret_cast<JCoreObject* const*>(fieldPtr);
            out.tag = REValueTag::ObjectUUID;
            out.s = o ? o->GetUUID() : "";
            return true;
        }

        default:
            return false;
    }
}

bool ReflectSerialize::TrySet(JCoreObject& obj, const REProperty& prop, const REVariant& in)
{
    void* basePtr = &obj;
    void* fieldPtr = prop.getPtr ? prop.getPtr(basePtr) : nullptr;
    if (!fieldPtr) return false;

    switch (prop.kind)
    {
        case REPropKind::Value:
            return TrySetValueByTypeName(fieldPtr, prop.typeName, in);

        case REPropKind::Enum:
        {
            if (!prop.enumType) return false;
            if (in.tag != REValueTag::EnumInt64) return false;

            const size_t sz = EnumUnderlyingSizeBytes(*prop.enumType);
            int64_t v = in.i64;
            std::memcpy(fieldPtr, &v, std::min(sz, sizeof(v)));
            return true;
        }

        case REPropKind::ObjectPtr:
        {
            if (in.tag != REValueTag::ObjectUUID) return false;

            JCoreObject* resolved = nullptr;
            if (!in.s.empty() && g_ObjectResolver)
                resolved = g_ObjectResolver(in.s);

            *reinterpret_cast<JCoreObject**>(fieldPtr) = resolved;
            return true;
        }

        default:
            return false;
    }
}

bool ReflectSerialize::ShouldSerializeProperty(const REProperty &p, RESerializeMode mode)
{
    const auto& S = REMetaSchema::Get();

    if (S.Has(p.meta, REMetaID::SkipSerialization)) return false;
    if (S.Has(p.meta, REMetaID::Transient)) return false;

    if (mode == RESerializeMode::SaveGameOnly)
        return S.Has(p.meta, REMetaID::SaveGame);

    return true;
}

// ---------------- Main dispatch ----------------

void ReflectSerialize::SerializeTypeProperties(JsonWriter& writer, const REType& type, const void* mostPtr)
{
    const RETypeRegistry& reg = RETypeRegistry::Get();

    std::vector<const REType*> chain;
    for (auto t = &type; t != nullptr; t = reg.GetBaseType(t))
        chain.push_back(t);
    std::reverse(chain.begin(), chain.end());

    for (const REType* t : chain)
    {
        const void* thisPtr = (t == &type) ? mostPtr
                                  : (t->upcastFromMostDerived ? t->upcastFromMostDerived(mostPtr) : mostPtr);

        for (const REProperty& prop : t->properties)
            SerializeProperty(writer, prop, thisPtr);
    }
}

void ReflectSerialize::DeserializeTypeProperties(const JsonReader& reader, const REType& type, void* mostPtr)
{
    const RETypeRegistry& reg = RETypeRegistry::Get();

    std::vector<const REType*> chain;
    for (auto t = &type; t != nullptr; t = reg.GetBaseType(t))
        chain.push_back(t);
    std::reverse(chain.begin(), chain.end());

    for (const REType* t : chain)
    {
        void* thisPtr = mostPtr;
        if (t != &type && t->upcastFromMostDerived)
            thisPtr = const_cast<void*>(t->upcastFromMostDerived(mostPtr));

        for (const REProperty& prop : t->properties)
            DeserializeProperty(reader, prop, thisPtr);
    }
}

// --- Array comparisons added to CDO checker ---
bool ReflectSerialize::IsPropertyOverridden(const REProperty& prop, const void* instBase, const void* cdoBase)
{
    const void* a = prop.getConstPtr ? prop.getConstPtr(instBase) : nullptr;
    const void* b = prop.getConstPtr ? prop.getConstPtr(cdoBase)  : nullptr;
    if (!a || !b) return false;

    switch (prop.kind)
    {
        case REPropKind::Value:
            return !EqualsValueByTypeName(a, b, prop.typeName);

        case REPropKind::Enum:
        {
            const size_t sz = EnumUnderlyingSizeBytes(*prop.enumType);
            return std::memcmp(a, b, sz) != 0;
        }

        case REPropKind::ObjectPtr:
        {
            auto* oa = *reinterpret_cast<JCoreObject* const*>(a);
            auto* ob = *reinterpret_cast<JCoreObject* const*>(b);
            const std::string ua = oa ? oa->GetUUID() : "";
            const std::string ub = ob ? ob->GetUUID() : "";
            return ua != ub;
        }

        case REPropKind::ReflectedStruct:
        {
            bool anyDiff = false;
            RETypeRegistry::Get().ForEachProperty_BaseToDerived(prop.reflectedType, [&](const REType&, const REProperty& sp)
            {
                if (anyDiff) return;
                anyDiff = IsPropertyOverridden(sp, a, b);
            });
            return anyDiff;
        }

        case REPropKind::Array:
        {
            if (!prop.array.size || !prop.array.getConst) return true;

            size_t countA = prop.array.size(const_cast<void*>(a));
            size_t countB = prop.array.size(const_cast<void*>(b));
            if (countA != countB) return true;

            for (size_t i = 0; i < countA; ++i)
            {
                const void* ea = prop.array.getConst(a, i);
                const void* eb = prop.array.getConst(b, i);

                switch (prop.elementKind)
                {
                    case REPropKind::Value:
                        if (!EqualsValueByTypeName(ea, eb, prop.elementTypeName)) return true;
                        break;
                    case REPropKind::Enum:
                    {
                        const size_t sz = EnumUnderlyingSizeBytes(*prop.elementEnumType);
                        if (std::memcmp(ea, eb, sz) != 0) return true;
                        break;
                    }
                    case REPropKind::ObjectPtr:
                    {
                        auto* oa = *reinterpret_cast<JCoreObject* const*>(ea);
                        auto* ob = *reinterpret_cast<JCoreObject* const*>(eb);
                        if ((oa ? oa->GetUUID() : "") != (ob ? ob->GetUUID() : "")) return true;
                        break;
                    }
                    case REPropKind::ReflectedStruct:
                    {
                        bool anyDiff = false;
                        RETypeRegistry::Get().ForEachProperty_BaseToDerived(prop.elementReflectedType, [&](const REType&, const REProperty& sp)
                        {
                            if (anyDiff) return;
                            anyDiff = IsPropertyOverridden(sp, ea, eb);
                        });
                        if (anyDiff) return true;
                        break;
                    }
                    default:
                        return true; // Unknown type, assume overridden to be safe
                }
            }
            return false;
        }

        default: return false;
    }
}

void ReflectSerialize::SerializeProperty(JsonWriter& writer, const REProperty& prop, const void* basePtr)
{
    const char* fieldName = prop.name.c_str();
    const void* fieldPtr  = GetFieldPtrConst(prop, basePtr);

    switch (prop.kind)
    {
        case REPropKind::Value:
        {
            if (!WriteValueByTypeName(writer, fieldName, fieldPtr, prop.typeName))
            {
                std::cerr << "[JReflection]: SerializeProperty: unsupported Value type for "
                          << fieldName << " : " << prop.typeName << "\n";
            }
            break;
        }

        case REPropKind::ReflectedStruct:
        {
            if (!prop.reflectedType) break;

            writer.BeginObject(fieldName);
            SerializeTypeProperties(writer, *prop.reflectedType, fieldPtr);
            writer.EndObject();
            break;
        }

        case REPropKind::Enum:
        {
            if (!prop.enumType) break;
            const size_t sz = EnumUnderlyingSizeBytes(*prop.enumType);

            int64_t v = 0;
            std::memcpy(&v, fieldPtr, std::min(sz, sizeof(v)));
            writer.Write(fieldName, v);
            break;
        }

        case REPropKind::ObjectPtr:
        {
            const JCoreObject* obj = *reinterpret_cast<JCoreObject* const*>(fieldPtr);
            writer.Write(fieldName, obj ? obj->GetUUID() : "");
            break;
        }

        case REPropKind::Array:
        {
            if (!prop.array.size) break;
            const size_t count = prop.array.size(const_cast<void*>(fieldPtr));

            writer.BeginArray(fieldName);

            for (size_t i = 0; i < count; ++i)
            {
                const void* elemPtr = prop.array.getConst(fieldPtr, i);

                switch (prop.elementKind)
                {
                    case REPropKind::Value:
                    {
                        // --- Replaced key-based writer with array appender ---
                        WriteArrayElementByTypeName(writer, elemPtr, prop.elementTypeName);
                        break;
                    }
                    case REPropKind::ReflectedStruct:
                    {
                        writer.BeginObject();
                        SerializeTypeProperties(writer, *prop.elementReflectedType, elemPtr);
                        writer.EndObject();
                        break;
                    }
                    case REPropKind::Enum:
                    {
                        int64_t v = 0;
                        const size_t sz = EnumUnderlyingSizeBytes(*prop.elementEnumType);
                        std::memcpy(&v, elemPtr, std::min(sz, sizeof(v)));
                        writer.WriteValue(v);
                        break;
                    }
                    case REPropKind::ObjectPtr:
                    {
                        const JCoreObject* obj = *reinterpret_cast<JCoreObject* const*>(elemPtr);
                        writer.WriteValue(obj ? obj->GetUUID() : "");
                        break;
                    }
                    default:
                        break;
                }
            }

            writer.EndArray();
            break;
        }

        default:
            break;
    }
}

void ReflectSerialize::DeserializeProperty(const JsonReader& reader, const REProperty& prop, void* basePtr)
{
    const char* fieldName = prop.name.c_str();
    void* fieldPtr = GetFieldPtr(prop, basePtr);

    switch (prop.kind)
    {
        case REPropKind::Value:
        {
            ReadValueByTypeName(reader, fieldName, fieldPtr, prop.typeName);
            break;
        }

        case REPropKind::ReflectedStruct:
        {
            if (!prop.reflectedType) break;
            JsonReader sub = reader.GetObject(fieldName);
            if (!sub.IsValid()) break;
            DeserializeTypeProperties(sub, *prop.reflectedType, fieldPtr);
            break;
        }

        case REPropKind::Enum:
        {
            if (!prop.enumType) break;
            const size_t sz = EnumUnderlyingSizeBytes(*prop.enumType);

            int64_t cur = 0;
            int64_t v = reader.Read(fieldName, cur);

            std::memcpy(fieldPtr, &v, std::min(sz, sizeof(v)));
            break;
        }

        case REPropKind::ObjectPtr:
        {
            JCoreObject* current = *reinterpret_cast<JCoreObject**>(fieldPtr);

            std::string curUuid;
            if (current) curUuid = current->GetUUID();

            std::string uuid = reader.Read(fieldName, curUuid);
            if (uuid == curUuid) return;

            JCoreObject* resolved = nullptr;
            if (!uuid.empty() && g_ObjectResolver)
                resolved = g_ObjectResolver(uuid);

            *reinterpret_cast<JCoreObject**>(fieldPtr) = resolved;
            break;
        }

        case REPropKind::Array:
        {
            std::vector<JsonReader> elementReaders = reader.GetArray(fieldName);
            size_t count = elementReaders.size();

            if (!prop.array.resize || !prop.array.get) {
                std::cerr << "[JReflection] Array property '" << prop.name
                          << "' is missing resize/get accessors\n";
                break;
            }

            prop.array.resize(fieldPtr, count);

            for (size_t i = 0; i < count; ++i)
            {
                void* elemPtr = prop.array.get(fieldPtr, i);
                if (!elemPtr) continue;

                JsonReader& elemReader = elementReaders[i];

                switch (prop.elementKind)
                {
                    case REPropKind::Value:
                    {
                        // --- Replaced key-based reader with direct element reader ---
                        ReadValueFromJson(elemReader.GetData(), elemPtr, prop.elementTypeName);
                        break;
                    }
                    case REPropKind::ReflectedStruct:
                    {
                        if (prop.elementReflectedType)
                            DeserializeTypeProperties(elemReader, *prop.elementReflectedType, elemPtr);
                        break;
                    }
                    case REPropKind::Enum:
                    {
                        int64_t raw = elemReader.GetData().get<int64_t>();
                        size_t sz = EnumUnderlyingSizeBytes(*prop.elementEnumType);
                        std::memcpy(elemPtr, &raw, sz);
                        break;
                    }
                    case REPropKind::ObjectPtr:
                    {
                        std::string uuid = elemReader.GetData().get<std::string>();
                        JCoreObject* resolved = uuid.empty() ? nullptr :
                            (g_ObjectResolver ? g_ObjectResolver(uuid) : nullptr);

                        *reinterpret_cast<JCoreObject**>(elemPtr) = resolved;
                        break;
                    }
                    default:
                        break;
                }
            }
            break;
        }

        default:
            break;
    }
}

void ReflectSerialize::SerializeReflectedProperties(JsonWriter& writer, const JCoreObject& obj)
{
    const REType* mostDerived = obj.GetREType();
    if (!mostDerived) mostDerived = RETypeRegistry::Get().FindType(typeid(obj));
    if (!mostDerived) return;

    SerializeTypeProperties(writer, *mostDerived, &obj);
}

void ReflectSerialize::DeserializeReflectedProperties(const JsonReader& reader, JCoreObject& obj)
{
    const REType* mostDerived = obj.GetREType();
    if (!mostDerived) mostDerived = RETypeRegistry::Get().FindType(typeid(obj));
    if (!mostDerived) return;

    DeserializeTypeProperties(reader, *mostDerived, &obj);
}

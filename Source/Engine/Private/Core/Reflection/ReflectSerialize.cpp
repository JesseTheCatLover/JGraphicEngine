//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Reflection/ReflectSerialize.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "Core/JCoreObject.h"
#include "Core/Reflection/RETypeRegistry.h"

// ---------------- Resolver ----------------

static ReflectSerialize::ResolveObjectByUUIDFn g_ObjectResolver = nullptr;

void ReflectSerialize::SetObjectResolver(ResolveObjectByUUIDFn fn)
{
    g_ObjectResolver = fn;
}

// ---------------- Small string helpers ----------------

static std::string StripSpaces(std::string s)
{
    s.erase(std::remove_if(s.begin(), s.end(),
        [](unsigned char c) { return std::isspace(c) != 0; }), s.end());
    return s;
}

static bool TypeNameIsPointer(const std::string& tn)
{
    return tn.find('*') != std::string::npos;
}

static std::string StripPointerStars(std::string s)
{
    s.erase(std::remove(s.begin(), s.end(), '*'), s.end());
    return s;
}

// ---------------- Enum helpers ----------------

static size_t EnumUnderlyingSizeBytes(const REEnum& e)
{
    // Underlying type is raw tokens from generator, e.g. "uint8_t" or "unsigned int"
    std::string u = StripSpaces(e.underlyingType);

    if (u.empty())
    {
        // If generator didn’t capture underlying type, assume int (common default)
        return sizeof(int);
    }

    // Common cases
    if (u == "uint8_t"  || u == "unsignedchar" || u == "unsigned__int8") return 1;
    if (u == "int8_t"   || u == "signedchar"   || u == "__int8")          return 1;

    if (u == "uint16_t" || u == "unsignedshort" || u == "unsigned__int16") return 2;
    if (u == "int16_t"  || u == "short"         || u == "__int16")         return 2;

    if (u == "uint32_t" || u == "unsignedint"   || u == "unsigned__int32") return 4;
    if (u == "int32_t"  || u == "int"           || u == "__int32")         return 4;

    if (u == "uint64_t" || u == "unsignedlonglong" || u == "unsigned__int64") return 8;
    if (u == "int64_t"  || u == "longlong"         || u == "__int64")          return 8;

    // Fallback: treat like int
    return sizeof(int);
}

static const char* EnumFindNameByValue(const REEnum& e, int64_t v)
{
    // MVP: enum table stores valueExpr as raw string, so we cannot evaluate expressions yet.
    // For now we prefer name-based IO only when deserializing from string,
    // and serialize enums as their NAME if possible only when we already have the name.
    // (Later we can generate numeric values directly into REEnumValue.)
    (void)e;
    (void)v;
    return nullptr;
}

static bool EnumTryParseByName(const REEnum& e, const std::string& name, int64_t& outValue)
{
    // Same MVP limitation: we don’t have numeric values computed.
    // If generator later emits numeric values, implement lookup here.
    //
    // For now: if valueExpr is a plain integer literal, we can parse it.
    for (const auto& ev : e.values)
    {
        if (ev.name == name)
        {
            if (!ev.valueExpr.empty())
            {
                // Very permissive parse (base-10 only)
                char* end = nullptr;
                long long parsed = std::strtoll(ev.valueExpr.c_str(), &end, 10);
                if (end && *end == '\0')
                {
                    outValue = (int64_t)parsed;
                    return true;
                }
            }

            // If no valueExpr (or not parseable), we can’t reconstruct numeric.
            // Returning false forces the caller to skip assignment.
            return false;
        }
    }
    return false;
}

// ---------------- Value IO (typeName-based) ----------------
// Helper: write a single property value based on its typeName

static bool WriteValueByTypeName(JsonWriter& writer, const char* name, const void* fieldPtr, const std::string& typeName)
{
    // Primitive examples; extend as needed.
    if (typeName == "int" || typeName == "int32" || typeName == "int32_t")
    {
        writer.Write(name, *reinterpret_cast<const int*>(fieldPtr));
        return true;
    }
    if (typeName == "size_t" || typeName == "std::size_t")
    {
        writer.Write(name, *reinterpret_cast<const size_t*>(fieldPtr));
        return true;
    }
    if (typeName == "float")
    {
        writer.Write(name, *reinterpret_cast<const float*>(fieldPtr));
        return true;
    }
    if (typeName == "double")
    {
        writer.Write(name, *reinterpret_cast<const double*>(fieldPtr));
        return true;
    }
    if (typeName == "bool")
    {
        writer.Write(name, *reinterpret_cast<const bool*>(fieldPtr));
        return true;
    }
    if (typeName == "std::string" || typeName == "string")
    {
        writer.Write(name, *reinterpret_cast<const std::string*>(fieldPtr));
        return true;
    }

    // Engine math types
    if (typeName == "FVector2")
    {
        writer.WriteVect2(name, *reinterpret_cast<const FVector2*>(fieldPtr));
        return true;
    }
    if (typeName == "FVector3")
    {
        writer.WriteVect3(name, *reinterpret_cast<const FVector3*>(fieldPtr));
        return true;
    }
    if (typeName == "FVector4")
    {
        writer.WriteVect4(name, *reinterpret_cast<const FVector4*>(fieldPtr));
        return true;
    }
    if (typeName == "FMatrix4")
    {
        writer.WriteMatrix4(name, *reinterpret_cast<const FMatrix4*>(fieldPtr));
        return true;
    }
    if (typeName == "FRotator")
    {
        writer.WriteRotator(name, *reinterpret_cast<const FRotator*>(fieldPtr));
        return true;
    }
    if (typeName == "FQuat")
    {
        writer.WriteQuat(name, *reinterpret_cast<const FQuat*>(fieldPtr));
        return true;
    }
    if (typeName == "FTransform")
    {
        writer.WriteTransform(name, *reinterpret_cast<const FTransform*>(fieldPtr));
        return true;
    }

    return false;
}

// Helper: read a single property value based on its typeName
static bool ReadValueByTypeName(const JsonReader& reader, const char* name, void* fieldPtr, const std::string& typeName)
{
    if (typeName == "int" || typeName == "int32" || typeName == "int32_t")
    {
        auto& ref = *reinterpret_cast<int*>(fieldPtr);
        ref = reader.Read(name, ref);
        return true;
    }
    if (typeName == "size_t" || typeName == "std::size_t")
    {
        auto& ref = *reinterpret_cast<size_t*>(fieldPtr);
        ref = reader.Read(name, ref);
        return true;
    }
    if (typeName == "float")
    {
        auto& ref = *reinterpret_cast<float*>(fieldPtr);
        ref = reader.Read(name, ref);
        return true;
    }
    if (typeName == "double")
    {
        auto& ref = *reinterpret_cast<double*>(fieldPtr);
        ref = reader.Read(name, ref);
        return true;
    }
    if (typeName == "bool")
    {
        auto& ref = *reinterpret_cast<bool*>(fieldPtr);
        ref = reader.Read(name, ref);
        return true;
    }
    if (typeName == "std::string" || typeName == "string")
    {
        auto& ref = *reinterpret_cast<std::string*>(fieldPtr);
        ref = reader.Read(name, ref);
        return true;
    }

    if (typeName == "FVector2")
    {
        auto& ref = *reinterpret_cast<FVector2*>(fieldPtr);
        ref = reader.ReadVector2(name, ref);
        return true;
    }
    if (typeName == "FVector3")
    {
        auto& ref = *reinterpret_cast<FVector3*>(fieldPtr);
        ref = reader.ReadVector3(name, ref);
        return true;
    }
    if (typeName == "FVector4")
    {
        auto& ref = *reinterpret_cast<FVector4*>(fieldPtr);
        ref = reader.ReadVector4(name, ref);
        return true;
    }
    if (typeName == "FMatrix4")
    {
        auto& ref = *reinterpret_cast<FMatrix4*>(fieldPtr);
        ref = reader.ReadMatrix4(name, ref);
        return true;
    }
    if (typeName == "FRotator")
    {
        auto& ref = *reinterpret_cast<FRotator*>(fieldPtr);
        ref = reader.ReadRotator(name, ref);
        return true;
    }
    if (typeName == "FQuat")
    {
        auto& ref = *reinterpret_cast<FQuat*>(fieldPtr);
        ref = reader.ReadQuat(name, ref);
        return true;
    }
    if (typeName == "FTransform")
    {
        auto& ref = *reinterpret_cast<FTransform*>(fieldPtr);
        ref = reader.ReadTransform(name, ref);
        return true;
    }

    return false;
}

// ---------------- Main dispatch ----------------

void ReflectSerialize::SerializeTypeProperties(JsonWriter& writer, const REType& type, const void* basePtr)
{
    // Walk from derived up through all base types (uses registry graph)
    const RETypeRegistry& reg = RETypeRegistry::Get();

    for (const REType* t = &type; t != nullptr; t = reg.GetBaseType(t))
    {
        for (const REProperty& prop : t->properties)
        {
            SerializeProperty(writer, prop, basePtr);
        }
    }
}

void ReflectSerialize::DeserializeTypeProperties(const JsonReader& reader, const REType& type, void* basePtr)
{
    const RETypeRegistry& reg = RETypeRegistry::Get();

    for (const REType* t = &type; t != nullptr; t = reg.GetBaseType(t))
    {
        for (const REProperty& prop : t->properties)
        {
            DeserializeProperty(reader, prop, basePtr);
        }
    }
}

void ReflectSerialize::SerializeProperty(JsonWriter& writer, const REProperty& prop, const void* basePtr)
{
    const char* fieldName = prop.name.c_str();
    const char* byteBase = reinterpret_cast<const char*>(basePtr);
    const void* fieldPtr = byteBase + prop.offset;

    switch (prop.kind)
    {
    case REPropKind::Value:
    {
        if (!WriteValueByTypeName(writer, fieldName, fieldPtr, prop.typeName))
        {
            std::cerr << "[JReflection]: SerializeReflectedProperties: unsupported Value type for "
                      << fieldName << " : " << prop.typeName << "\n";
        }
        break;
    }

    case REPropKind::ReflectedStruct:
    {
        if (!prop.reflectedType)
        {
            std::cerr << "[JReflection]: SerializeReflectedProperties: ReflectedStruct missing reflectedType for "
                      << fieldName << "\n";
            break;
        }

        // Struct is embedded by value; fieldPtr points at the struct memory
        SerializeTypeProperties(writer, *prop.reflectedType, fieldPtr);
        break;
    }

    case REPropKind::Enum:
    {
        if (!prop.enumType)
        {
            std::cerr << "[JReflection]: SerializeReflectedProperties: Enum missing enumType for "
                      << fieldName << "\n";
            break;
        }

        // Serialize enum as underlying integer (MVP-safe)
        // We do not rely on evaluating valueExpr yet.
        const size_t sz = EnumUnderlyingSizeBytes(*prop.enumType);

        int64_t v = 0;
        std::memcpy(&v, fieldPtr, std::min(sz, sizeof(v)));

        writer.Write(fieldName, v);
        break;
    }

    case REPropKind::ObjectPtr:
    {
        // Serialize object reference as UUID string (empty for null)
        // Field is expected to be a pointer-sized slot (e.g. JActor*)
        const JCoreObject* obj = nullptr;

        // prop.typeName could be "JActor*" or "JCoreObject *"
        // We treat it as "JCoreObject*" storage here.
        obj = *reinterpret_cast<JCoreObject* const*>(fieldPtr);

        std::string uuid;
        if (obj)
            uuid = obj->GetUUID();

        writer.Write(fieldName, uuid);
        break;
    }

    default:
        // Unknown => do nothing
        break;
    }
}

void ReflectSerialize::DeserializeProperty(const JsonReader& reader, const REProperty& prop, void* basePtr)
{
    const char* fieldName = prop.name.c_str();
    char* byteBase = reinterpret_cast<char*>(basePtr);
    void* fieldPtr = byteBase + prop.offset;

    switch (prop.kind)
    {
    case REPropKind::Value:
    {
        if (!ReadValueByTypeName(reader, fieldName, fieldPtr, prop.typeName))
        {
            // keep silent by default to avoid spam, or log in debug
            // std::cerr << "[JReflection]: DeserializeReflectedProperties: unsupported Value type for "
            //           << fieldName << " : " << prop.typeName << "\n";
        }
        break;
    }

    case REPropKind::ReflectedStruct:
    {
        if (!prop.reflectedType)
            break;

        DeserializeTypeProperties(reader, *prop.reflectedType, fieldPtr);
        break;
    }

    case REPropKind::Enum:
    {
        if (!prop.enumType)
            break;

        // Read underlying integer and memcpy into enum storage
        const size_t sz = EnumUnderlyingSizeBytes(*prop.enumType);

        int64_t cur = 0;
        int64_t v = reader.Read(fieldName, cur);

        std::memcpy(fieldPtr, &v, std::min(sz, sizeof(v)));
        break;
    }

    case REPropKind::ObjectPtr:
    {
        // Read UUID string and resolve it to pointer (if resolver is set)
        std::string cur;
        std::string uuid = reader.Read(fieldName, cur);

        JCoreObject* resolved = nullptr;
        if (!uuid.empty() && g_ObjectResolver)
            resolved = g_ObjectResolver(uuid);

        *reinterpret_cast<JCoreObject**>(fieldPtr) = resolved;
        break;
    }

    default:
        break;
    }
}

void ReflectSerialize::SerializeReflectedProperties(JsonWriter& writer, const JCoreObject& obj)
{
    const REType* mostDerived = obj.GetREType();
    if (!mostDerived)
    {
        // fallback (should rarely happen if codegen is correct)
        mostDerived = RETypeRegistry::Get().FindType(typeid(obj));
    }

    if (!mostDerived)
        return;

    SerializeTypeProperties(writer, *mostDerived, &obj);
}

void ReflectSerialize::DeserializeReflectedProperties(const JsonReader& reader, JCoreObject& obj)
{
    const REType* mostDerived = obj.GetREType();
    if (!mostDerived)
    {
        mostDerived = RETypeRegistry::Get().FindType(typeid(obj));
    }

    if (!mostDerived)
        return;

    DeserializeTypeProperties(reader, *mostDerived, &obj);
}
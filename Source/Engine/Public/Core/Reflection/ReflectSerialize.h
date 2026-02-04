//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <functional>
#include <string>

class JsonReader;
class JsonWriter;

class JCoreObject;
struct REType;
struct REProperty;

/**
 * Reflection-based serialization for JCoreObject and reflected structs.
 *
 * Uses RETypeRegistry Finalize() results:
 *  - REPropKind::Value           => serialize by typeName (built-in value IO)
 *  - REPropKind::ReflectedStruct => recursively serialize using reflectedType
 *  - REPropKind::Enum            => serialize using enum table (string name by default)
 *  - REPropKind::ObjectPtr       => serialize object reference (UUID), resolved by callback
 */
class ReflectSerialize
{
private:
    friend class JCoreObject;

    // Serialize all reflected properties of a reflected object
    static void SerializeReflectedProperties(JsonWriter& writer, const JCoreObject& obj);

    // Deserialize all reflected properties of a reflected object
    static void DeserializeReflectedProperties(const JsonReader& reader, JCoreObject& obj);

public:
    // ---------------- Object reference resolving ----------------
    // During load we need a way to turn UUID -> object pointer.
    // The engine (SerializationSubsystem/SceneManager) should set this once at startup.
    using ResolveObjectByUUIDFn = JCoreObject*(*)(const std::string& uuid);

    static void SetObjectResolver(ResolveObjectByUUIDFn fn);

private:
    // Internal helpers (work for classes and reflected structs)
    static void SerializeTypeProperties(JsonWriter& writer, const REType& type, const void* basePtr);
    static void DeserializeTypeProperties(const JsonReader& reader, const REType& type, void* basePtr);

    static void SerializeProperty(JsonWriter& writer, const REProperty& prop, const void* basePtr);
    static void DeserializeProperty(const JsonReader& reader, const REProperty& prop, void* basePtr);
};
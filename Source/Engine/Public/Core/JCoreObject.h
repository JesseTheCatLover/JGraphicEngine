//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <cstring>

#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

#include "Core/Reflection/JReflectionMacro.h"
#include "Core/Reflection/JTypeRegistry.h"

#include "Utilities/UUUID.h"

class JReflectionRegistrar;

// Helper macros to pick the first argument if given, otherwise default
#define DECLARE_JOBJECT_1(Type) DECLARE_JOBJECT_IMPL(Type, JCoreObject)
#define DECLARE_JOBJECT_2(Type, BaseType) DECLARE_JOBJECT_IMPL(Type, BaseType)

// Main macro that dispatches based on number of arguments
#define GET_MACRO(_1,_2,NAME,...) NAME
#define DECLARE_JOBJECT(...) GET_MACRO(__VA_ARGS__, DECLARE_JOBJECT_2, DECLARE_JOBJECT_1)(__VA_ARGS__)

// Implementation
#define DECLARE_JOBJECT_IMPL(Type, BaseType) \
public: \
    static const char* StaticTypeName() { return #Type; } \
    const char* GetClassTypeName() const override { return #Type; } \
    using Super = BaseType; \
    friend void _JRegister_##Type(JReflectionRegistrar&);         \
private:

class JCoreObject
{
public:
    virtual ~JCoreObject() = default;

    // Type info
    virtual const char* GetClassTypeName() const = 0;

    template<typename T>
    [[nodiscard]] bool IsA() const
    {
        return std::strcmp(GetClassTypeName(), T::StaticTypeName()) == 0;
    }

    // Every core object had a runtime ID for fast runtime lookup
    [[nodiscard]] uint64_t GetRuntimeID() const { return m_ID; }
    void SetRuntimeID(const uint64_t id) { m_ID = id; }

    // Unique ID across all save/load sessions
    [[nodiscard]] const std::string& GetUUID() const { return m_UUID; }

protected:
    JCoreObject()
        : m_ID(++m_NextID)
    { // assign unique ID at construction
        m_UUID = UUUID::GenerateUUID();
    }

    // Serialization hooks
    virtual void Serialize(class JsonWriter& writer) const {}
    virtual void Deserialize(const class JsonReader& reader) {}

private:
    uint64_t m_ID; // runtime-only ID
    std::string m_UUID; // serialized stable and unique ID

    inline static uint64_t m_NextID = 0; // global counter

    void SerializeJObject(class JsonWriter& writer) const
    {
        writer.BeginObject("test");
        writer.Write("uuid", m_UUID);
        writer.EndObject();

        Serialize(writer);
    }
    void DeserializeJObject(const class JsonReader& reader)
    {
        m_UUID = reader.Read("uuid", m_UUID);

        Deserialize(reader);
    }
};

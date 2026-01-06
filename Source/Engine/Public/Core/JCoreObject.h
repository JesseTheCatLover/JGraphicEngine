//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <cstring>

#include "Serialization/SerializeUtilities.h"

#include "Reflection/JReflectionMacro.h"
#include "Reflection/RETypeRegistry.h"

#include "Reflection/JReflectionSerialization.h"

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

/**
 * @class JCoreObject
 * @brief Base class for all engine objects that support identity, reflection, and serialization.
 *
 * JCoreObject provides:
 *  - A unique runtime ID (fast integer)
 *  - A persistent UUID used for load/save identification
 *  - Engine-level type information via DECLARE_JOBJECT()
 *  - Automatic reflection-based serialization for all JPROPERTY() fields
 *
 * Derived classes may override SerializeCustom() and DeserializeCustom() to handle
 * non-reflected or special-case data.
 */
class JCoreObject
{
    friend class SerializationSubsystem;
    friend class SceneManager;

public:
    virtual ~JCoreObject() = default;

    // Type info
    [[nodiscard]] virtual const char* GetClassTypeName() const = 0;

    template<typename T>
    [[nodiscard]] bool IsA() const
    {
        return std::strcmp(GetClassTypeName(), T::StaticTypeName()) == 0;
    }

    // Every core object had a runtime ID for fast runtime lookup
    [[nodiscard]] uint64_t GetRuntimeID() const { return m_ID; }
    void SetRuntimeID(const uint64_t id) { m_ID = id; }

    // Unique UUID across all save/load sessions
    [[nodiscard]] const std::string& GetUUID() const { return m_UUID; }

protected:
    JCoreObject()
        : m_ID(++m_NextID)
    { // assign unique ID at construction
        m_UUID = UUUID::GenerateUUID();
    }

    /**
     * Custom (manual) serialization hook, for serializing objects out of the reflection system.
     * @param writer Writer utility
     */
    virtual void SerializeCustom(class JsonWriter& writer) const {}

    /**
     * Custom (manual) deserialization hook, for deserializing objects out of the reflection system.
     * @param reader Reader utility
     */
    virtual void DeserializeCustom(const class JsonReader& reader) {}

    /**
     * Called after deserialization. Objects may construct, populate data and allocate resources using this hook.
     */
    virtual void PostLoad() {};

private:
    uint64_t m_ID; ///< runtime-only ID
    std::string m_UUID; ///< serialized stable and unique UUID

    inline static uint64_t m_NextID = 0; // global counter for runtime IDs

    /**
     * Reflective serialization on each JCoreObject derived class.
     * @param writer Writer utility
     */
    void SerializeJObject(class JsonWriter& writer) const
    {
        // Reflective serialization:
        JReflectionSerialization::SerializeReflectedProperties(writer, *this);

        // Custom (manual) serialization hook:
        SerializeCustom(writer);
    }

    /**
     * Reflective deserialization on each JCoreObject derived class.
     * @param reader Reader utility
     */
    void DeserializeJObject(const class JsonReader& reader)
    {
        // Reflective deserialization:
        JReflectionSerialization::DeserializeReflectedProperties(reader, *this);

        // Custom hook:
        DeserializeCustom(reader);

        PostLoad();
    }
};
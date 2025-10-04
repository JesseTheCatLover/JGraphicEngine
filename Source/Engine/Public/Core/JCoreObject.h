//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <string>

// TODO: instead of an expensive RTTI we do this for now
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
private:

class JCoreObject
{
public:
    virtual ~JCoreObject() = default;

    // Type info
    virtual const char* GetClassTypeName() const = 0;

    // Every core object has a unique ID
    uint64_t GetID() const { return m_ID; }

    // Serialization hooks
    virtual void Serialize(class JsonWriter& writer) const = 0;
    virtual void Deserialize(const class JsonReader& reader) = 0;

protected:
    JCoreObject()
        : m_ID(++s_NextID) {} // assign unique ID at construction

private:
    uint64_t m_ID; // engine-unique object ID
    static uint64_t s_NextID; // global counter
};

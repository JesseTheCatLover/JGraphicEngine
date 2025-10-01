//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <string>

// TODO: instead of an expensive RTTI we do this for now
#define DECLARE_JCLASS(Type) \
public: \
static const char* StaticTypeName() { return #Type; } \
const char* GetClassTypeName() const override { return #Type; } \
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

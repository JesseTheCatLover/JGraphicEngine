//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Reflection/JReflectionSerialization.h"
#include "Core/JCoreObject.h"
#include <iostream>

// Helper: write a single property value based on its type
static void WritePropertyValue(JsonWriter& writer, const char* name, const void* fieldPtr, const std::type_index& typeIndex)
{
    // Primitive examples; extend as needed.
    if (typeIndex == typeid(int))
    {
        writer.Write(name, *reinterpret_cast<const int*>(fieldPtr));
    }
    else if (typeIndex == typeid(float))
    {
        writer.Write(name, *reinterpret_cast<const float*>(fieldPtr));
    }
    else if (typeIndex == typeid(double))
    {
        writer.Write(name, *reinterpret_cast<const double*>(fieldPtr));
    }
    else if (typeIndex == typeid(bool))
    {
        writer.Write(name, *reinterpret_cast<const bool*>(fieldPtr));
    }
    else if (typeIndex == typeid(std::string))
    {
        writer.Write(name, *reinterpret_cast<const std::string*>(fieldPtr));
    }
    else
    {
        // TODO: handle engine types like FVector3, FRotator, enums, etc.
        // For now we just skip unsupported types.
        // std::cerr << "[JReflection]: SerializeReflectedProperties: unsupported type for " << name << "\n";
    }
}

// Helper: read a single property value based on its type
static void ReadPropertyValue(const JsonReader& reader, const char* name, void* fieldPtr, const std::type_index& typeIndex)
{
    if (typeIndex == typeid(int))
    {
        auto& ref = *reinterpret_cast<int*>(fieldPtr);
        ref = reader.Read(name, ref);
    }
    else if (typeIndex == typeid(float))
    {
        auto& ref = *reinterpret_cast<float*>(fieldPtr);
        ref = reader.Read(name, ref);
    }
    else if (typeIndex == typeid(double))
    {
        auto& ref = *reinterpret_cast<double*>(fieldPtr);
        ref = reader.Read(name, ref);
    }
    else if (typeIndex == typeid(bool))
    {
        auto& ref = *reinterpret_cast<bool*>(fieldPtr);
        ref = reader.Read(name, ref);
    }
    else if (typeIndex == typeid(std::string))
    {
        auto& ref = *reinterpret_cast<std::string*>(fieldPtr);
        ref = reader.Read(name, ref);
    }
    else
    {
        // TODO: handle engine types like FVector3, FRotator, enums, etc.
        // std::cerr << "[JReflection]: DeserializeReflectedProperties: unsupported type for " << name << "\n";
    }
}

void JReflectionSerialization::SerializeReflectedProperties(JsonWriter& writer, const JCoreObject& obj)
{
    const REType* type = RETypeRegistry::FindType(typeid(obj));
    if (!type)
        return;

    const char* base = reinterpret_cast<const char*>(&obj);

    for (const REProperty& prop : type->properties)
    {
        const char* fieldName = prop.name.c_str();
        const void* fieldPtr  = base + prop.offset;

        WritePropertyValue(writer, fieldName, fieldPtr, prop.type);
    }
}

void JReflectionSerialization::DeserializeReflectedProperties(const JsonReader &reader, JCoreObject &obj)
{
    const REType* type = RETypeRegistry::FindType(typeid(obj));
    if (!type)
        return;

    char* base = reinterpret_cast<char*>(&obj);

    for (const REProperty& prop : type->properties)
    {
        const char* fieldName = prop.name.c_str();
        void* fieldPtr = base + prop.offset;

        ReadPropertyValue(reader, fieldName, fieldPtr, prop.type);
    }
}

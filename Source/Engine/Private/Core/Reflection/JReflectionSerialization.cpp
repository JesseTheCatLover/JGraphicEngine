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
    else if (typeIndex == typeid(size_t))
    {
        writer.Write(name, *reinterpret_cast<const size_t*>(fieldPtr));
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
    else if (typeIndex == typeid(FVector2))
    {
        writer.WriteVect2(name, *reinterpret_cast<const FVector2*>(fieldPtr));
    }
    else if (typeIndex == typeid(FVector3))
    {
        writer.WriteVect3(name, *reinterpret_cast<const FVector3*>(fieldPtr));
    }
    else if (typeIndex == typeid(FVector4))
    {
        writer.WriteVect4(name, *reinterpret_cast<const FVector4*>(fieldPtr));
    }
    else if (typeIndex == typeid(FMatrix4))
    {
        writer.WriteMatrix4(name, *reinterpret_cast<const FMatrix4*>(fieldPtr));
    }
    else if (typeIndex == typeid(FRotator))
    {
        writer.WriteRotator(name, *reinterpret_cast<const FRotator*>(fieldPtr));
    }
    else if (typeIndex == typeid(FQuat))
    {
        writer.WriteQuat(name, *reinterpret_cast<const FQuat*>(fieldPtr));
    }
    else if (typeIndex == typeid(FTransform))
    {
        writer.WriteTransform(name, *reinterpret_cast<const FTransform*>(fieldPtr));
    }
    else
    {
        // TODO: Handle enums
        // For now we just skip unsupported types.
        std::cerr << "[JReflection]: SerializeReflectedProperties: unsupported type for " << name << "\n";
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
    else if (typeIndex == typeid(size_t))
    {
        auto& ref = *reinterpret_cast<size_t*>(fieldPtr);
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
    else if (typeIndex == typeid(FVector2))
    {
        auto& ref = *reinterpret_cast<FVector2*>(fieldPtr);
        ref = reader.ReadVector2(name, ref);
    }
    else if (typeIndex == typeid(FVector3))
    {
        auto& ref = *reinterpret_cast<FVector3*>(fieldPtr);
        ref = reader.ReadVector3(name, ref);
    }
    else if (typeIndex == typeid(FVector4))
    {
        auto& ref = *reinterpret_cast<FVector4*>(fieldPtr);
        ref = reader.ReadVector4(name, ref);
    }
    else if (typeIndex == typeid(FMatrix4))
    {
        auto& ref = *reinterpret_cast<FMatrix4*>(fieldPtr);
        ref = reader.ReadMatrix4(name, ref); // assumes you added ReadMatrix4
    }
    else if (typeIndex == typeid(FRotator))
    {
        auto& ref = *reinterpret_cast<FRotator*>(fieldPtr);
        ref = reader.ReadRotator(name, ref);
    }
    else if (typeIndex == typeid(FQuat))
    {
        auto& ref = *reinterpret_cast<FQuat*>(fieldPtr);
        ref = reader.ReadQuat(name, ref);
    }
    else if (typeIndex == typeid(FTransform))
    {
        auto& ref = *reinterpret_cast<FTransform*>(fieldPtr);
        ref = reader.ReadTransform(name, ref);
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

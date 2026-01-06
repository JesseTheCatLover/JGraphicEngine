//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "RETypeRegistry.h"
#include "Core/Reflection/JReflectionMetadata.h"
#include "Core/Serialization/SerializeUtilities.h"
#include <string>
#include <typeinfo>

class JCoreObject;

class JReflectionSerialization
{
private:
    friend class JCoreObject;

    // Serialize all reflected properties of a reflected object
    static void SerializeReflectedProperties(JsonWriter& writer, const JCoreObject& obj);

    // Deserialize all reflected properties of a reflected object
    static void DeserializeReflectedProperties(const JsonReader& reader, JCoreObject& obj);
};

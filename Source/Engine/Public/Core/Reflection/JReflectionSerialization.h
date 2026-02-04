//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Core/Reflection/RETypeRegistry.h"
#include "Core/Serialization/SerializeUtilities.h"

class JCoreObject;

class JReflectionSerialization
{
private:
    friend class JCoreObject;

    static void SerializeReflectedProperties(JsonWriter& writer, const JCoreObject& obj);
    static void DeserializeReflectedProperties(const JsonReader& reader, JCoreObject& obj);
};
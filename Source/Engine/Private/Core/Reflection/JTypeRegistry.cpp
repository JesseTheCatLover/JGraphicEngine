//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Reflection/JTypeRegistry.h"
#include <utility>
std::unordered_map<std::type_index, REType> JTypeRegistry::s_types;

void JTypeRegistry::BeginType(const char* name, const std::type_info& typeInfo, const std::type_info& baseType)
{
    REType type;
    type.name = name;
    type.cppType = std::type_index(typeInfo);
    type.baseCppType = std::type_index(baseType);

    s_types[std::type_index(typeInfo)] = std::move(type);
}

//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "../../../Public/Core/Reflection/RETypeRegistry.h"
#include <utility>

std::unordered_map<std::type_index, REType> RETypeRegistry::s_types;

void RETypeRegistry::BeginType(const char* name, const std::type_info& typeInfo, const std::type_info& baseType)
{
    REType type;
    type.name = name;
    type.cppType = std::type_index(typeInfo);
    type.baseCppType = std::type_index(baseType);

    s_types[std::type_index(typeInfo)] = std::move(type);
}

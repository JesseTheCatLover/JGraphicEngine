//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Reflection/RETypeRegistry.h"

#include <iostream>
#include <utility>

std::unordered_map<std::type_index, REType> RETypeRegistry::s_Types;

void RETypeRegistry::BeginType(const char* name, const std::type_info& typeInfo, const std::type_info& baseType)
{
    auto typeIndex = std::type_index(typeInfo);
    REType& type = s_Types[typeIndex]; // create or reuse

    type.name = name;
    type.cppType = typeIndex;
    type.baseCppType = std::type_index(baseType);
}

void RETypeRegistry::DebugDumpAllTypes()
{
    std::cout << "=== Registered REType entries ===\n";
    for (auto& [idx, type] : RETypeRegistry::s_Types)
    {
        std::cout << "Type: " << type.name << "  base: ";
        if (type.baseCppType == std::type_index(typeid(void)))
        {
            std::cout << "<none>";
        }
        else
        {
            const REType* base = RETypeRegistry::FindType(type.baseCppType);
            std::cout << (base ? base->name : "<unregistered>");
        }
        std::cout << "\n";
    }
}


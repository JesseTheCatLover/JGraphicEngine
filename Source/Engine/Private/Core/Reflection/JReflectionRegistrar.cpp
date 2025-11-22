//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Reflection/JReflectionRegistrar.h"
#include <iostream>
#include "Scene/JActor.h"

JReflectionRegistrar::JReflectionRegistrar(const char* name, const std::type_info& typeInfo, const std::type_info& baseTypeInfo)
{
    // Engine's own registry side:
    RETypeRegistry::BeginType(name, typeInfo, baseTypeInfo);
}

static void DumpType(const std::type_info& ti)
{
    const REType* type = RETypeRegistry::FindType(ti);
    if (!type)
    {
        std::cout << "[JReflection]: Type not found in registry\n";
        return;
    }

    std::cout << "Type: " << type->name << "\n";
    std::cout << "Properties:\n";

    for (const REProperty& prop : type->properties)
    {
        std::cout << "  - " << prop.name
                  << " (cpp type = " << prop.type.name()
                  << ", offset = " << prop.offset << ")\n";

        for (const FMetaEntry& meta : prop.metadata.entries)
        {
            std::cout << "      meta kind = " << int(meta.kind)
                      << ", key = \""   << meta.key
                      << "\", value = \"" << meta.value << "\"\n";
        }
    }
}

void JReflectionRegistrar::DebugReflection_JActor()
{
    DumpType(typeid(JActor));
}
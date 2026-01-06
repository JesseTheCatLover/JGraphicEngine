//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <typeinfo>
#include "Core/Reflection/JReflectionMetaData.h"
#include "Core/Reflection/RETypeRegistry.h"

class JReflectionRegistrar
{
public:
    JReflectionRegistrar(const char* name, const std::type_info& typeInfo, const std::type_info& baseTypeInfo);

    template<typename SelfType, typename T>
    void AddProperty(const char* name, T SelfType::*member,
                     const FPropertyMetadata& meta)
    {
        // Engine's reflection:
        RETypeRegistry::AddProperty<SelfType, T>(name, member, meta);
    }

    static void DebugReflection_JActor();
};

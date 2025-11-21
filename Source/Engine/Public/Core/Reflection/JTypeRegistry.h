//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <unordered_map>
#include <typeindex>
#include "Core/Reflection/JReflectionMetaData.h"

struct REProperty
{
    std::string name;
    std::type_index type;
    size_t offset;
    FPropertyMetadata metadata;
};

struct REType
{
    std::string name;
    std::type_index cppType{ typeid(void) };
    std::type_index baseCppType{ typeid(void) };
    std::vector<REProperty> properties;
};

class JTypeRegistry
{
public:
    static void BeginType(const char* name,
                          const std::type_info& typeInfo,
                          const std::type_info& baseType);

    template<typename SelfType, typename T>
    static void AddProperty(const char* propName, T SelfType::*member,
                            const FPropertyMetadata& metadata)
    {
        auto& t = s_types[std::type_index(typeid(SelfType))];
        t.properties.push_back(REProperty{
            propName,
            std::type_index(typeid(T)),
            size_t(&( ((SelfType*)0)->*member )),  // offset
            metadata
        });
    }

    static const REType* FindType(const std::type_info& typeInfo)
    {
        auto it = s_types.find(std::type_index(typeInfo));
        return (it != s_types.end()) ? &it->second : nullptr;
    }

    template<typename T>
    static const REType* GetType()
    {
        return FindType(typeid(T));
    }

private:
    static std::unordered_map<std::type_index, REType> s_types;
};

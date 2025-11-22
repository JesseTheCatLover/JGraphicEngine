//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <typeindex>
#include "Core/Reflection/JReflectionMetaData.h"

class JCoreObject;

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
    std::function<JCoreObject*()> factory;
};

class RETypeRegistry
{
public:
    static void BeginType(const char* name, const std::type_info& typeInfo, const std::type_info& baseType);

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

    static const REType* FindTypeByTypeName(const std::string& name)
    {
        for (auto& [key, value] : s_types)
        {
            if (value.name == name)
                return &value;
        }
        return nullptr;
    }

    /**
     * @brief Attach a default factory for T (new T()).
     * Call this once when registering the type.
     **/
    template<typename T>
    static void SetFactory()
    {
        auto it = s_types.find(std::type_index(typeid(T)));
        if (it != s_types.end())
        {
            it->second.factory = []() -> JCoreObject*
            {
                return new T();
            };
        }
    }

    static JCoreObject* CreateInstanceByTypeName(const std::string& name)
    {
        const REType* type = FindTypeByTypeName(name);
        if (!type || !type->factory)
            return nullptr;
        return type->factory();
    }

private:
    static std::unordered_map<std::type_index, REType> s_types;
};

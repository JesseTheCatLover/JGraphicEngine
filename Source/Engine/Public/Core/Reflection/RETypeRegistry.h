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
        auto& t = s_Types[std::type_index(typeid(SelfType))];
        t.properties.push_back(REProperty{
            propName,
            std::type_index(typeid(T)),
            size_t(&( ((SelfType*)0)->*member )),  // offset
            metadata
        });
    }

    static const REType* FindType(const std::type_index& idx)
    {
        auto it = s_Types.find(idx);
        return (it != s_Types.end()) ? &it->second : nullptr;
    }

    static const REType* FindType(const std::type_info& typeInfo)
    {
        return FindType(std::type_index(typeInfo));
    }

    template<typename T>
    static const REType* GetType()
    {
        return FindType(typeid(T));
    }

    // Small helper to walk up the inheritance chain
    static const REType* GetBaseType(const REType* type)
    {
        if (!type)
            return nullptr;

        // Sentinel for "no base"
        if (type->baseCppType == std::type_index(typeid(void)))
            return nullptr;

        return FindType(type->baseCppType);
    }

    static const REType* FindTypeByTypeName(const std::string& name)
    {
        for (auto& [key, value] : s_Types)
        {
            if (value.name == name)
                return &value;
        }
        return nullptr;
    }

    // Generic: set factory from any callable
    template<typename T, typename FactoryFn>
    static void SetFactory(FactoryFn fn)
    {
        auto typeIndex = std::type_index(typeid(T));
        auto it = s_Types.find(typeIndex);
        if (it == s_Types.end())
            it = s_Types.emplace(typeIndex, REType{}).first;

        it->second.factory = fn;
    }

    // Default: new T() (for concrete types)
    template<typename T>
    static void SetDefaultFactory()
    {
        if constexpr (std::is_abstract_v<T>)
        {
            ClearFactory<T>();
        }
        else
        {
            SetFactory<T>([]() -> JCoreObject*
            {
                return new T();
            });
        }
    }

    template<typename T>
    static void ClearFactory()
    {
        auto ti = std::type_index(typeid(T));
        auto it = s_Types.find(ti);
        if (it == s_Types.end())
            return;

        it->second.factory = nullptr;
    }

    static JCoreObject* CreateInstanceByTypeName(const std::string& name)
    {
        const REType* type = FindTypeByTypeName(name);
        if (!type || !type->factory)
            return nullptr;
        return type->factory();
    }

    static void DebugDumpAllTypes();

private:
    static std::unordered_map<std::type_index, REType> s_Types;
};

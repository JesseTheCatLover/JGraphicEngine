//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "REMeta.h"

class JCoreObject;

// ---------------- Reflection structures ----------------

struct REProperty
{
    std::string name;

    std::string typeName;

    size_t offset = 0;

    REMetaList meta;
};

struct REFunction
{
    std::string name;

    // Raw signature strings are perfect for MVP:
    // e.g. "void()", "const Vec3&() const noexcept", etc.
    std::string signature;

    // Later: add invoker pointer, flags, param info, etc.
    uint32_t flags = 0;

    REMetaList meta;
};

struct REEnumValue
{
    std::string name;
    std::string valueExpr; // raw expression string (optional)
};

struct REEnum
{
    std::string name;
    bool isScoped = false;
    std::string underlyingType; // raw, optional
    REMetaList meta;
    std::vector<REEnumValue> values;
};

struct REType
{
    std::string name;

    std::type_index cppType{ typeid(void) };
    std::type_index baseCppType{ typeid(void) };

    REMetaList meta; // class-level attributes from JCLASS/JSTRUCT

    std::vector<REProperty> properties;
    std::vector<REFunction> functions;

    // Optional: if this is an enum type, store enum data here
    // (We can also store enums separately; this is convenient for lookup-by-name)
    bool isEnum = false;
    REEnum enumInfo;

    std::function<JCoreObject*()> factory;
};

class RETypeRegistry
{
public:
    // ---------------- Lifetime / access ----------------
    static RETypeRegistry& Get();

    // ---------------- Registration API (used by generated .refl.gen.cpp) ----------------

    // Register a type (class/struct) with RTTI identity + base type
    void BeginType(const char* name,
                   const std::type_info& selfType,
                   const std::type_info& baseType);

    // Add class-level meta (from JCLASS/JSTRUCT args)
    void AddTypeMeta(const std::type_info& ownerType,
                     const char* key,
                     const char* value = "");

    // Property registration by raw offset
    void AddProperty(const std::type_info& ownerType,
                     const char* propName,
                     const char* propTypeName,
                     size_t offset);

    // Optional legacy convenience overload (not used by codegen)
    void AddProperty(const std::type_info& ownerType,
                     const char* propName,
                     const std::type_info& propType,
                     size_t offset);

    void AddPropertyMeta(const std::type_info& ownerType,
                         const char* propName,
                         const char* key,
                         const char* value = "");

    // Function registration (no invoker in MVP)
    void AddFunction(const std::type_info& ownerType,
                     const char* funcName,
                     const char* signature,
                     uint32_t flags = 0);

    void AddFunctionMeta(const std::type_info& ownerType,
                         const char* funcName,
                         const char* signature,
                         const char* key,
                         const char* value = "");

    // Enum registration (optional but included for full integration)
    void BeginEnum(const char* name,
                   bool isScoped,
                   const char* underlyingType = "");

    void AddEnumMeta(const char* enumName,
                     const char* key,
                     const char* value = "");

    void AddEnumValue(const char* enumName,
                      const char* valueName,
                      const char* valueExpr = "");

    // Factory registration
    void SetFactory(const std::type_info& ownerType,
                    std::function<JCoreObject*()> factory);

    // ---------------- Lookup ----------------

    const REType* FindType(const std::type_info& ti) const;
    const REType* FindType(const std::type_index& idx) const;

    const REType* FindTypeByName(const std::string& name) const;

    // Inheritance helpers
    const REType* GetBaseType(const REType* type) const;

    bool IsDerivedFrom(const REType* type, const REType* base) const;

    // Factory helper
    JCoreObject* CreateInstanceByTypeName(const std::string& name) const;

    // Debug
    void DebugDumpAllTypes() const;

private:
    RETypeRegistry() = default;

    // internal helpers
    REType& EnsureTypeEntry(const std::type_index& idx);
    REType* FindTypeMutable(const std::type_index& idx);

    // For name->type lookup we store type_index to avoid raw pointers invalidation concerns.
    std::unordered_map<std::type_index, REType> m_Types;
    std::unordered_map<std::string, std::type_index> m_NameToType;

    // Enums are name-driven (no RTTI necessarily)
    std::unordered_map<std::string, REEnum> m_Enums;
};
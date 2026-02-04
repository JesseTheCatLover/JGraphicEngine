//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Reflection/RETypeRegistry.h"

#include <iostream>

RETypeRegistry& RETypeRegistry::Get()
{
    static RETypeRegistry g;
    return g;
}

REType& RETypeRegistry::EnsureTypeEntry(const std::type_index& idx)
{
    auto it = m_Types.find(idx);
    if (it != m_Types.end())
        return it->second;

    auto [insIt, _] = m_Types.emplace(idx, REType{});
    insIt->second.cppType = idx;
    return insIt->second;
}

REType* RETypeRegistry::FindTypeMutable(const std::type_index& idx)
{
    auto it = m_Types.find(idx);
    return (it != m_Types.end()) ? &it->second : nullptr;
}

void RETypeRegistry::BeginType(const char* name,
                              const std::type_info& selfType,
                              const std::type_info& baseType)
{
    const auto selfIdx = std::type_index(selfType);
    const auto baseIdx = std::type_index(baseType);

    REType& t = EnsureTypeEntry(selfIdx);
    t.name = name;
    t.cppType = selfIdx;
    t.baseCppType = baseIdx;

    // Don't use operator[] (std::type_index has no default ctor)
    m_NameToType.insert_or_assign(t.name, selfIdx);
}

void RETypeRegistry::AddTypeMeta(const std::type_info& ownerType,
                                const char* key,
                                const char* value)
{
    const auto ownerIdx = std::type_index(ownerType);
    REType& t = EnsureTypeEntry(ownerIdx);
    REAddMeta(t.meta, key ? key : "", value ? value : "");
}

void RETypeRegistry::AddProperty(const std::type_info& ownerType,
                                const char* propName,
                                const char* propTypeName,
                                size_t offset)
{
    auto ownerIdx = std::type_index(ownerType);
    REType& t = EnsureTypeEntry(ownerIdx);

    REProperty p;
    p.name = propName ? propName : "";
    p.typeName = propTypeName ? propTypeName : "";
    p.offset = offset;

    t.properties.push_back(std::move(p));
}

void RETypeRegistry::AddProperty(const std::type_info& ownerType,
                                const char* propName,
                                const std::type_info& propType,
                                size_t offset)
{
    // convenience wrapper
    AddProperty(ownerType, propName, propType.name(), offset);
}

void RETypeRegistry::AddPropertyMeta(const std::type_info& ownerType,
                                     const char* propName,
                                     const char* key,
                                     const char* value)
{
    const auto ownerIdx = std::type_index(ownerType);
    REType& t = EnsureTypeEntry(ownerIdx);

    for (auto& p : t.properties)
    {
        if (p.name == (propName ? propName : ""))
        {
            REAddMeta(p.meta, key ? key : "", value ? value : "");
            return;
        }
    }

    // If metadata arrives before property (shouldn't happen, but be robust)
    REProperty p;
    p.name = propName ? propName : "";
    REAddMeta(p.meta, key ? key : "", value ? value : "");
    t.properties.push_back(std::move(p));
}

void RETypeRegistry::AddFunction(const std::type_info& ownerType,
                                const char* funcName,
                                const char* signature,
                                uint32_t flags)
{
    const auto ownerIdx = std::type_index(ownerType);
    REType& t = EnsureTypeEntry(ownerIdx);

    REFunction f;
    f.name = funcName ? funcName : "";
    f.signature = signature ? signature : "";
    f.flags = flags;

    t.functions.push_back(std::move(f));
}

void RETypeRegistry::AddFunctionMeta(const std::type_info& ownerType,
                                    const char* funcName,
                                    const char* signature,
                                    const char* key,
                                    const char* value)
{
    const auto ownerIdx = std::type_index(ownerType);
    REType& t = EnsureTypeEntry(ownerIdx);

    const std::string n = funcName ? funcName : "";
    const std::string sig = signature ? signature : "";

    for (auto& f : t.functions)
    {
        if (f.name == n && f.signature == sig)
        {
            REAddMeta(f.meta, key ? key : "", value ? value : "");
            return;
        }
    }

    // Robust fallback: create stub entry
    REFunction f;
    f.name = n;
    f.signature = sig;
    REAddMeta(f.meta, key ? key : "", value ? value : "");
    t.functions.push_back(std::move(f));
}

void RETypeRegistry::BeginEnum(const char* name, bool isScoped, const char* underlyingType)
{
    const std::string en = name ? name : "";
    REEnum& e = m_Enums[en];
    e.name = en;
    e.isScoped = isScoped;
    e.underlyingType = underlyingType ? underlyingType : "";
}

void RETypeRegistry::AddEnumMeta(const char* enumName,
                                const char* key,
                                const char* value)
{
    const std::string en = enumName ? enumName : "";
    REEnum& e = m_Enums[en];
    e.name = en;
    REAddMeta(e.meta, key ? key : "", value ? value : "");
}

void RETypeRegistry::AddEnumValue(const char* enumName,
                                 const char* valueName,
                                 const char* valueExpr)
{
    const std::string en = enumName ? enumName : "";
    REEnum& e = m_Enums[en];
    e.name = en;

    REEnumValue v;
    v.name = valueName ? valueName : "";
    v.valueExpr = valueExpr ? valueExpr : "";
    e.values.push_back(std::move(v));
}

void RETypeRegistry::SetFactory(const std::type_info& ownerType,
                               std::function<JCoreObject*()> factory)
{
    const auto ownerIdx = std::type_index(ownerType);
    REType& t = EnsureTypeEntry(ownerIdx);
    t.factory = std::move(factory);
}

const REType* RETypeRegistry::FindType(const std::type_info& ti) const
{
    return FindType(std::type_index(ti));
}

const REType* RETypeRegistry::FindType(const std::type_index& idx) const
{
    auto it = m_Types.find(idx);
    return (it != m_Types.end()) ? &it->second : nullptr;
}

const REType* RETypeRegistry::FindTypeByName(const std::string& name) const
{
    auto it = m_NameToType.find(name);
    if (it == m_NameToType.end())
        return nullptr;

    return FindType(it->second);
}

const REType* RETypeRegistry::GetBaseType(const REType* type) const
{
    if (!type) return nullptr;
    if (type->baseCppType == std::type_index(typeid(void))) return nullptr;
    return FindType(type->baseCppType);
}

JCoreObject* RETypeRegistry::CreateInstanceByTypeName(const std::string& name) const
{
    const REType* t = FindTypeByName(name);
    if (!t || !t->factory)
        return nullptr;
    return t->factory();
}

void RETypeRegistry::DebugDumpAllTypes() const
{
    std::cout << "=== Registered RE Types ===\n";

    for (const auto& [idx, t] : m_Types)
    {
        std::cout << "Type: " << t.name << "\n";

        std::cout << "  Base: ";
        if (t.baseCppType == std::type_index(typeid(void)))
            std::cout << "<none>\n";
        else
        {
            const REType* base = FindType(t.baseCppType);
            std::cout << (base ? base->name : "<unregistered>") << "\n";
        }

        if (!t.meta.empty())
        {
            std::cout << "  Meta:\n";
            for (auto& m : t.meta)
                std::cout << "    - " << m.key << (m.value.empty() ? "" : ("=" + m.value)) << "\n";
        }

        if (!t.properties.empty())
        {
            std::cout << "  Properties:\n";
            for (auto& p : t.properties)
            {
                std::cout << "    - " << p.name << " : " << p.typeName
                          << " (offset=" << p.offset << ")\n";
                for (auto& m : p.meta)
                    std::cout << "        meta: " << m.key << (m.value.empty() ? "" : ("=" + m.value)) << "\n";
            }
        }

        if (!t.functions.empty())
        {
            std::cout << "  Functions:\n";
            for (auto& f : t.functions)
            {
                std::cout << "    - " << f.name << "  sig: " << f.signature << "  flags=" << f.flags << "\n";
                for (auto& m : f.meta)
                    std::cout << "        meta: " << m.key << (m.value.empty() ? "" : ("=" + m.value)) << "\n";
            }
        }

        std::cout << "\n";
    }

    if (!m_Enums.empty())
    {
        std::cout << "=== Registered Enums ===\n";
        for (auto& [name, e] : m_Enums)
        {
            std::cout << "Enum: " << e.name
                      << (e.isScoped ? " (scoped)" : "")
                      << (e.underlyingType.empty() ? "" : (" : " + e.underlyingType))
                      << "\n";

            for (auto& m : e.meta)
                std::cout << "  meta: " << m.key << (m.value.empty() ? "" : ("=" + m.value)) << "\n";

            for (auto& v : e.values)
                std::cout << "  - " << v.name << (v.valueExpr.empty() ? "" : (" = " + v.valueExpr)) << "\n";

            std::cout << "\n";
        }
    }
}
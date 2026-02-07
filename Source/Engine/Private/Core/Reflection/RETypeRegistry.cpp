//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Reflection/RETypeRegistry.h"

#include <algorithm>
#include <iostream>

static const std::type_index kVoidType = std::type_index(typeid(void));

RETypeRegistry& RETypeRegistry::Get()
{
    static RETypeRegistry g;
    return g;
}

REType& RETypeRegistry::EnsureTypeEntry(const std::type_index& idx)
{
    auto it = m_Types.find(idx);
    if (it != m_Types.end())
        return *it->second;

    auto ptr = std::make_unique<REType>();
    ptr->cppType = idx;

    REType& ref = *ptr;
    m_Types.emplace(idx, std::move(ptr));
    return ref;
}

REType* RETypeRegistry::FindTypeMutable(const std::type_index& idx)
{
    auto it = m_Types.find(idx);
    return (it != m_Types.end()) ? it->second.get() : nullptr;
}

void RETypeRegistry::BeginType(const char* name,
                              RETypeKind kind,
                              const std::type_info& selfType,
                              const std::type_info& baseType)
{
    const auto selfIdx = std::type_index(selfType);
    const auto baseIdx = std::type_index(baseType);

    REType& t = EnsureTypeEntry(selfIdx);
    t.name = name ? name : "";
    t.kind = kind;
    t.cppType = selfIdx;
    t.baseCppType = baseIdx;

    // Store pointer (stable because REType is heap-allocated)
    if (!t.name.empty())
        m_NameToType.insert_or_assign(t.name, &t);
}

void RETypeRegistry::SetUpcast(const std::type_info& selfType, REUpcastFn upcastFn)
{
    REType& t = EnsureTypeEntry(std::type_index(selfType));
    t.upcastFromMostDerived = upcastFn;
}

void RETypeRegistry::AddTypeMeta(const std::type_info& ownerType,
                                const char* key,
                                const char* value)
{
    const auto ownerIdx = std::type_index(ownerType);
    REType& t = EnsureTypeEntry(ownerIdx);

    if (!key || !*key)
        return;

    REAddMeta(t.meta, key, value ? value : "");
}

void RETypeRegistry::AddPropertyMeta(const std::type_info& ownerType,
                                     const char* propName,
                                     const char* key,
                                     const char* value)
{
    const auto ownerIdx = std::type_index(ownerType);
    REType& t = EnsureTypeEntry(ownerIdx);

    if (!propName || !*propName || !key || !*key)
        return;

    // Attach to most recent matching property (supports repeated names in edge cases)
    for (auto it = t.properties.rbegin(); it != t.properties.rend(); ++it)
    {
        if (it->name == propName)
        {
            REAddMeta(it->meta, key, value ? value : "");
            return;
        }
    }

#ifndef NDEBUG
    std::cerr << "[RETypeRegistry] AddPropertyMeta: property not found: " << propName
              << " on type: " << t.name << "\n";
#endif
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

    if (!funcName || !*funcName || !key || !*key)
        return;

    const std::string n = funcName;
    const std::string sig = signature ? signature : "";

    for (auto it = t.functions.rbegin(); it != t.functions.rend(); ++it)
    {
        if (it->name == n && it->signature == sig)
        {
            REAddMeta(it->meta, key, value ? value : "");
            return;
        }
    }

#ifndef NDEBUG
    std::cerr << "[RETypeRegistry] AddFunctionMeta: function not found: " << n
              << " sig: " << sig << " on type: " << t.name << "\n";
#endif
}

void RETypeRegistry::BeginEnum(const char* name, bool isScoped, const char* underlyingType)
{
    const std::string en = name ? name : "";
    if (en.empty())
        return;

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
    if (en.empty() || !key || !*key)
        return;

    REEnum& e = m_Enums[en];
    e.name = en;
    REAddMeta(e.meta, key, value ? value : "");
}

void RETypeRegistry::AddEnumValue(const char* enumName,
                                 const char* valueName,
                                 const char* valueExpr)
{
    const std::string en = enumName ? enumName : "";
    if (en.empty() || !valueName || !*valueName)
        return;

    REEnum& e = m_Enums[en];
    e.name = en;

    REEnumValue v;
    v.name = valueName;
    v.valueExpr = valueExpr ? valueExpr : "";

    e.values.push_back(std::move(v));
}

void RETypeRegistry::SetFactory(const std::type_info& ownerType,
    std::function<JCoreObject*(const FObjectInitializer&)> factory)
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
    return (it != m_Types.end()) ? it->second.get() : nullptr;
}

const REType* RETypeRegistry::FindTypeByName(const std::string& name) const
{
    auto it = m_NameToType.find(name);
    return (it != m_NameToType.end()) ? it->second : nullptr;
}

const REEnum* RETypeRegistry::FindEnumByName(const std::string& name) const
{
    auto it = m_Enums.find(name);
    return (it != m_Enums.end()) ? &it->second : nullptr;
}

const REType* RETypeRegistry::GetBaseType(const REType* type) const
{
    if (!type) return nullptr;
    if (type->baseCppType == kVoidType) return nullptr;
    return FindType(type->baseCppType);
}

bool RETypeRegistry::IsDerivedFrom(const REType* type, const REType* base) const
{
    if (!type || !base) return false;
    for (auto t = type; t != nullptr; t = GetBaseType(t))
        if (t == base) return true;
    return false;
}

JCoreObject* RETypeRegistry::CreateInstanceByTypeName(const std::string& name,
                                                      const FObjectInitializer& Init) const
{
    const REType* T = FindTypeByName(name);
    if (!T)
    {
#ifndef NDEBUG
        std::cerr << "[RETypeRegistry] CreateInstanceByTypeName: type not found: " << name << "\n";
#endif
        return nullptr;
    }

    if (!T->factory)
    {
#ifndef NDEBUG
        std::cerr << "[RETypeRegistry] CreateInstanceByTypeName: no factory for type: " << name << "\n";
        // Optional: print common meta reasons if you want
        for (const auto& m : T->meta)
        {
            if (m.key == "Abstract" || m.key == "NoSpawnCtor" || m.key == "NoFactory")
                std::cerr << "  reason meta: " << m.key << (m.value.empty() ? "" : ("=" + m.value)) << "\n";
        }
#endif
        return nullptr;
    }

    return T->factory(Init);
}

// ---------------- Finalization / resolution ----------------

// Very MVP heuristics. We can later upgrade to token parsing for types.
static bool TypeLooksLikePointer(const std::string& tn)
{
    return tn.find('*') != std::string::npos;
}

static std::string StripSpaces(std::string s)
{
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
    return s;
}

static std::string StripPointerStars(std::string s)
{
    s.erase(std::remove(s.begin(), s.end(), '*'), s.end());
    return s;
}

static std::string Trim(std::string s)
{
    auto is_ws = [](unsigned char c){ return std::isspace(c) != 0; };
    while (!s.empty() && is_ws((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && is_ws((unsigned char)s.back()))  s.pop_back();
    return s;
}

static void ReplaceAll(std::string& s, const std::string& from, const std::string& to)
{
    if (from.empty()) return;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos)
    {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
}

static std::string NormalizeTypeName(std::string s)
{
    // 1) collapse whitespace
    s = Trim(s);
    // cheap collapse: remove all spaces then reinsert none (your registry uses exact names anyway)
    // BUT we *do* want to preserve namespaces and templates: removing spaces is fine.
    s = StripSpaces(std::move(s));

    // 2) remove common cv/ref qualifiers (after space-strip)
    // examples:
    //   "constFObjectInitializer&" -> "FObjectInitializer"
    //   "FObjectInitializerconst&" -> "FObjectInitializer"
    ReplaceAll(s, "const", "");
    ReplaceAll(s, "volatile", "");
    ReplaceAll(s, "&&", "");
    ReplaceAll(s, "&", "");

    // 3) remove leading tag keywords that sometimes appear in raw token strings
    ReplaceAll(s, "class", "");
    ReplaceAll(s, "struct", "");
    ReplaceAll(s, "enum", "");

    s = Trim(s);
    return s;
}
void RETypeRegistry::ResolvePropertyKinds(REType& owner)
{
    for (auto& p : owner.properties)
    {
        p.kind = REPropKind::Unknown;
        p.reflectedType = nullptr;
        p.enumType = nullptr;
        p.objectType = nullptr;

        const std::string raw = p.typeName;
        const std::string norm = NormalizeTypeName(raw);

        // 1) Enum by name
        if (const REEnum* e = FindEnumByName(norm))
        {
            p.kind = REPropKind::Enum;
            p.enumType = e;
            continue;
        }

        // 2) Reflected struct/class by name
        if (const REType* rt = FindTypeByName(norm))
        {
            if (rt->kind == RETypeKind::Struct)
            {
                p.kind = REPropKind::ReflectedStruct;
                p.reflectedType = rt;
            }
            else
            {
                // value-member of reflected class is weird; keep as Value for now
                p.kind = REPropKind::Value;
                p.reflectedType = rt;
            }
            continue;
        }

        // 3) Pointer: treat as object pointer if pointee is a reflected class
        if (TypeLooksLikePointer(raw))
        {
            // strip spaces then strip '*', then normalize again for const/etc
            std::string base = StripPointerStars(StripSpaces(raw));
            base = NormalizeTypeName(base);

            if (const REType* objT = FindTypeByName(base))
            {
                // Only treat reflected *classes* as ObjectPtr.
                // (Struct pointers can be "Value" or a later feature; your choice.)
                if (objT->kind == RETypeKind::Class)
                {
                    p.kind = REPropKind::ObjectPtr;
                    p.objectType = objT;
                    continue;
                }
            }
        }

        // 4) Default
        p.kind = REPropKind::Value;
    }
}

void RETypeRegistry::Finalize()
{
    // Resolve property kinds for all types
    for (auto& [idx, uptr] : m_Types)
    {
        ResolvePropertyKinds(*uptr);

        // If this REType is an Enum type, link enum payload by name (optional)
        if (uptr->kind == RETypeKind::Enum)
        {
            if (const REEnum* e = FindEnumByName(uptr->name))
                uptr->enumInfo = e;
        }
    }
}

// ---------------- Debug ----------------

void RETypeRegistry::DebugDumpAllTypes() const
{
    std::cout << "=== Registered RE Types ===\n";

    for (const auto& [idx, uptr] : m_Types)
    {
        const REType& t = *uptr;

        std::cout << "Type: " << t.name << "\n";

        std::cout << "  Kind: ";
        switch (t.kind)
        {
        case RETypeKind::Class:  std::cout << "Class\n"; break;
        case RETypeKind::Struct: std::cout << "Struct\n"; break;
        case RETypeKind::Enum:   std::cout << "Enum\n"; break;
        }

        std::cout << "  Base: ";
        if (t.baseCppType == kVoidType)
            std::cout << "<none>\n";
        else
        {
            const REType* base = FindType(t.baseCppType);
            std::cout << (base ? base->name : "<unregistered>") << "\n";
        }

        if (!t.meta.empty())
        {
            std::cout << "  Meta:\n";
            for (const auto& m : t.meta)
                std::cout << "    - " << m.key << (m.value.empty() ? "" : ("=" + m.value)) << "\n";
        }

        if (!t.properties.empty())
        {
            std::cout << "  Properties:\n";
            for (const auto& p : t.properties)
            {
                std::cout << "    - " << p.name << " : " << p.typeName;

                const bool hasGetter = (bool)p.getPtr && (bool)p.getConstPtr;
                std::cout << (hasGetter ? "  [memberptr]" : "  [no-accessor]") << "\n";

                std::cout << "      kind: ";
                switch (p.kind)
                {
                    case REPropKind::Value:           std::cout << "Value\n"; break;
                    case REPropKind::ReflectedStruct: std::cout << "ReflectedStruct\n"; break;
                    case REPropKind::Enum:            std::cout << "Enum\n"; break;
                    case REPropKind::ObjectPtr:       std::cout << "ObjectPtr\n"; break;
                    default:                          std::cout << "Unknown\n"; break;
                }

                for (const auto& m : p.meta)
                    std::cout << "        meta: " << m.key << (m.value.empty() ? "" : ("=" + m.value)) << "\n";
            }
        }

        if (!t.functions.empty())
        {
            std::cout << "  Functions:\n";
            for (const auto& f : t.functions)
            {
                std::cout << "    - " << f.name << "  sig: " << f.signature << "  flags=" << f.flags << "\n";
                for (const auto& m : f.meta)
                    std::cout << "        meta: " << m.key << (m.value.empty() ? "" : ("=" + m.value)) << "\n";
            }
        }

        if (t.kind == RETypeKind::Enum && t.enumInfo)
        {
            const REEnum& e = *t.enumInfo;
            std::cout << "  EnumInfo: " << e.name
                      << (e.isScoped ? " (scoped)" : "")
                      << (e.underlyingType.empty() ? "" : (" : " + e.underlyingType))
                      << "\n";
            for (const auto& v : e.values)
                std::cout << "    - " << v.name << (v.valueExpr.empty() ? "" : (" = " + v.valueExpr)) << "\n";
        }

        std::cout << "\n";
    }

    if (!m_Enums.empty())
    {
        std::cout << "=== Registered Enums ===\n";
        for (const auto& [name, e] : m_Enums)
        {
            std::cout << "Enum: " << e.name
                      << (e.isScoped ? " (scoped)" : "")
                      << (e.underlyingType.empty() ? "" : (" : " + e.underlyingType))
                      << "\n";

            for (const auto& m : e.meta)
                std::cout << "  meta: " << m.key << (m.value.empty() ? "" : ("=" + m.value)) << "\n";

            for (const auto& v : e.values)
                std::cout << "  - " << v.name << (v.valueExpr.empty() ? "" : (" = " + v.valueExpr)) << "\n";

            std::cout << "\n";
        }
    }
}
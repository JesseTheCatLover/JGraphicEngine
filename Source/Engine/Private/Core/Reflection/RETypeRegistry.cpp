//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Reflection/RETypeRegistry.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <cstring>
#include <cstdlib>

#include "Core/FObjectInitTLS.h"
#include "Core/JCoreObject.h"

static const std::type_index kVoidType = std::type_index(typeid(void));

RETypeRegistry& RETypeRegistry::Get()
{
    static RETypeRegistry g;
    return g;
}

void RETypeRegistry::EnumRaw_FromI64(REVariant &v, int64_t value, uint8_t size, bool bSignedness)
{
    v.tag = REValueTag::EnumInt64; // keep tag for transport, but ALSO fill enumRaw
    v.i64 = value;

    v.enumRaw.size = size;
    v.enumRaw.signedness = bSignedness;

    auto tmp = (uint64_t)value;
    const size_t n = (size == 0) ? 8 : std::min<size_t>(size, 8);
    std::memset(v.enumRaw.bytes, 0, sizeof(v.enumRaw.bytes));
    std::memcpy(v.enumRaw.bytes, &tmp, n);
}

int64_t RETypeRegistry::EnumRaw_ToI64(const REVariant &v)
{
    // Prefer enumRaw if present, fallback to i64.
    if (v.enumRaw.size == 0)
        return v.i64;

    uint64_t tmp = 0;
    const size_t n = std::min<size_t>(v.enumRaw.size, 8);
    std::memcpy(&tmp, v.enumRaw.bytes, n);

    // sign extend if signed and smaller than 64
    if (v.enumRaw.signedness && n < 8)
    {
        const uint64_t signBit = 1ull << (n * 8 - 1);
        if (tmp & signBit)
            tmp |= ~((1ull << (n * 8)) - 1ull);
    }

    return (int64_t)tmp;
}

bool RETypeRegistry::ReadVariantFromProperty(const REProperty &prop, const void *basePtr, REVariant &out)
{
    out = {};

        const void* fieldPtr = prop.getConstPtr ? prop.getConstPtr(basePtr) : nullptr;
        if (!fieldPtr)
            return false;

        if (prop.kind == REPropKind::ObjectPtr)
        {
            out.tag = REValueTag::ObjectUUID;
            auto* obj = *reinterpret_cast<JCoreObject* const*>(fieldPtr);
            out.s = obj ? obj->GetUUID() : "";
            return true;
        }

        if (prop.kind == REPropKind::Enum)
        {
            // read EXACT underlying bytes
            const uint8_t size = prop.valueSize ? prop.valueSize : 8;
            const size_t n = std::min<size_t>(size, 8);

            out = {};
            out.tag = REValueTag::EnumInt64;
            out.enumRaw.size = size;
            out.enumRaw.signedness = prop.bSigned;

            std::memset(out.enumRaw.bytes, 0, sizeof(out.enumRaw.bytes));
            std::memcpy(out.enumRaw.bytes, fieldPtr, n);

            // compute i64 for UI lookup (sign-extend if needed)
            out.i64 = EnumRaw_ToI64(out);
            return true;
        }

        const std::string& tn = prop.typeName;

        if (tn == "bool")        { out.tag = REValueTag::Bool;   out.b = *reinterpret_cast<const bool*>(fieldPtr); return true; }
        if (tn == "int" || tn == "int32") { out.tag = REValueTag::Int; out.i32 = *reinterpret_cast<const int32_t*>(fieldPtr); return true; }
        if (tn == "int64")       { out.tag = REValueTag::Int64;  out.i64 = *reinterpret_cast<const int64_t*>(fieldPtr); return true; }
        if (tn == "size_t")      { out.tag = REValueTag::Int64;  out.i64 = (int64_t)*reinterpret_cast<const size_t*>(fieldPtr); return true; }
        if (tn == "float")       { out.tag = REValueTag::Float;  out.f32 = *reinterpret_cast<const float*>(fieldPtr); return true; }
        if (tn == "double")      { out.tag = REValueTag::Double; out.f64 = *reinterpret_cast<const double*>(fieldPtr); return true; }
        if (tn == "std::string") { out.tag = REValueTag::String; out.s   = *reinterpret_cast<const std::string*>(fieldPtr); return true; }

        if (tn == "FVector2")    { out.tag = REValueTag::Vec2; out.v2 = *reinterpret_cast<const FVector2*>(fieldPtr); return true; }
        if (tn == "FVector3")    { out.tag = REValueTag::Vec3; out.v3 = *reinterpret_cast<const FVector3*>(fieldPtr); return true; }
        if (tn == "FVector4")    { out.tag = REValueTag::Vec4; out.v4 = *reinterpret_cast<const FVector4*>(fieldPtr); return true; }
        if (tn == "FQuat")       { out.tag = REValueTag::Quat; out.q  = *reinterpret_cast<const FQuat*>(fieldPtr); return true; }
        if (tn == "FTransform")  { out.tag = REValueTag::Transform; out.t = *reinterpret_cast<const FTransform*>(fieldPtr); return true; }

        return false;
}

bool RETypeRegistry::ApplyVariantToProperty(const REProperty &prop, void *basePtr, const REVariant &v)
{
        if (prop.setFromValue)
            return prop.setFromValue(basePtr, v);

        void* fieldPtr = prop.getPtr ? prop.getPtr(basePtr) : nullptr;
        if (!fieldPtr) return false;

        const std::string& tn = prop.typeName;

        if (tn == "bool" && v.tag == REValueTag::Bool) { *reinterpret_cast<bool*>(fieldPtr) = v.b; return true; }

        if ((tn == "int" || tn == "int32") && v.tag == REValueTag::Int) { *reinterpret_cast<int32_t*>(fieldPtr) = v.i32; return true; }
        if (tn == "int64" && v.tag == REValueTag::Int64) { *reinterpret_cast<int64_t*>(fieldPtr) = v.i64; return true; }
        if (tn == "size_t" && v.tag == REValueTag::Int64) { *reinterpret_cast<size_t*>(fieldPtr) = (size_t)v.i64; return true; }

        if (tn == "float" && v.tag == REValueTag::Float)
        {
            float x = v.f32;
            const auto& rm = prop.GetResolvedMeta();
            if (rm.bHasClamp)
            {
                x = std::max(x, rm.clampMin);
                x = std::min(x, rm.clampMax);
            }
            *reinterpret_cast<float*>(fieldPtr) = x;
            return true;
        }

        if (tn == "double" && v.tag == REValueTag::Double) { *reinterpret_cast<double*>(fieldPtr) = v.f64; return true; }
        if (tn == "std::string" && v.tag == REValueTag::String) { *reinterpret_cast<std::string*>(fieldPtr) = v.s; return true; }

        if (tn == "FVector2" && v.tag == REValueTag::Vec2) { *reinterpret_cast<FVector2*>(fieldPtr) = v.v2; return true; }
        if (tn == "FVector3" && v.tag == REValueTag::Vec3) { *reinterpret_cast<FVector3*>(fieldPtr) = v.v3; return true; }
        if (tn == "FVector4" && v.tag == REValueTag::Vec4) { *reinterpret_cast<FVector4*>(fieldPtr) = v.v4; return true; }
        if (tn == "FQuat" && v.tag == REValueTag::Quat) { *reinterpret_cast<FQuat*>(fieldPtr) = v.q; return true; }
        if (tn == "FTransform" && v.tag == REValueTag::Transform) { *reinterpret_cast<FTransform*>(fieldPtr) = v.t; return true; }

        if (prop.kind == REPropKind::Enum && v.tag == REValueTag::EnumInt64)
        {
            const uint8_t size = prop.valueSize ? prop.valueSize : 8;
            const size_t  n    = std::min<size_t>(size, 8);

            // Prefer enumRaw (best), fallback to packing from i64
            uint8_t bytes[8]{};

            if (v.enumRaw.size != 0)
            {
                std::memcpy(bytes, v.enumRaw.bytes, n);
            }
            else
            {
                uint64_t tmp = (uint64_t)v.i64;
                std::memcpy(bytes, &tmp, n);
            }

            std::memcpy(fieldPtr, bytes, n);
            return true;
        }

        return false;
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
            it->InvalidateResolvedMetaCache();
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

void RETypeRegistry::AddEnumValueMeta(const char* enumName,
                                      const char* valueName,
                                      const char* key,
                                      const char* value)
{
    const std::string en = enumName ? enumName : "";
    if (en.empty() || !valueName || !*valueName || !key || !*key)
        return;

    auto it = m_Enums.find(en);
    if (it == m_Enums.end())
    {
        // Ensure enum exists even if meta arrives before BeginEnum/AddEnumValue
        REEnum& e = m_Enums[en];
        e.name = en;
        it = m_Enums.find(en);
    }

    REEnum& e = it->second;

    // Attach to most recent matching value (supports duplicates / reorders)
    for (auto vit = e.values.rbegin(); vit != e.values.rend(); ++vit)
    {
        if (vit->name == valueName)
        {
            REAddMeta(vit->meta, key, value ? value : "");
            return;
        }
    }

#ifndef NDEBUG
    std::cerr << "[RETypeRegistry] AddEnumValueMeta: value not found: " << valueName
              << " in enum: " << en << "\n";
#endif
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

JCoreObject* RETypeRegistry::GetCDO(const REType* type)
{
    if (!type) return nullptr;
    if (type->cdo) return type->cdo;
    if (!type->factory) return nullptr;

    // CDO initializer
    FObjectInitializer init = FObjectInitializer::ForCDO();

    // IMPORTANT: use factory through TLS path
    JCoreObject* obj = CreateInstanceByTypeName(type->name, init);
    type->cdo = obj;
    return obj;
}

JCoreObject* RETypeRegistry::CreateInstanceByTypeName(const std::string& name,
                                                      const FObjectInitializer& Init) const
{
    const REType* T = FindTypeByName(name);
    if (!T || !T->factory)
        return nullptr;

    // Local copy lives for the full construction call (safe for TLS pointers)
    FObjectInitializer LocalInit = Init;
    LocalInit.ConstructingObject = nullptr;

    FObjectInitTLS::FScope scope(LocalInit);
    return T->factory(LocalInit);
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

static bool TryParseInt64Literal(const std::string& s, int64_t& out)
{
    std::string t = Trim(s);
    if (t.empty())
        return false;

    // strip optional surrounding parentheses (common in macros)
    if (t.size() >= 2 && t.front() == '(' && t.back() == ')')
        t = Trim(t.substr(1, t.size() - 2));

    char* end = nullptr;
    long long v = std::strtoll(t.c_str(), &end, 0); // base=0 supports 0x..
    if (end == t.c_str())
        return false;

    // allow trailing u/U/l/L combos (very common)
    while (*end == 'u' || *end == 'U' || *end == 'l' || *end == 'L')
        ++end;

    while (*end && std::isspace((unsigned char)*end))
        ++end;

    if (*end != '\0')
        return false;

    out = (int64_t)v;
    return true;
}

static bool TryResolveEnumValueExpr_Simple(
    const std::string& expr,
    const std::unordered_map<std::string, int64_t>& resolved,
    int64_t& out)
{
    // MVP: either:
    //  1) integer literal (dec/hex)
    //  2) identifier referencing a previously resolved enumerator
    if (TryParseInt64Literal(expr, out))
        return true;

    const std::string id = Trim(expr);
    if (id.empty())
        return false;

    auto it = resolved.find(id);
    if (it == resolved.end())
        return false;

    out = it->second;
    return true;
}

void RETypeRegistry::ResolveEnumNumericValues()
{
    for (auto& [enumName, e] : m_Enums)
    {
        std::unordered_map<std::string, int64_t> resolved;
        resolved.reserve(e.values.size());

        int64_t nextImplicit = 0;

        for (auto& v : e.values)
        {
            int64_t value = nextImplicit;

            if (!v.valueExpr.empty())
            {
                int64_t tmp = 0;
                if (TryResolveEnumValueExpr_Simple(v.valueExpr, resolved, tmp))
                    value = tmp;
            }

            v.valueI64 = value;
            resolved[v.name] = value;
            nextImplicit = value + 1;
        }
    }
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
    // 0) Resolve enum numeric values FIRST (so UI can map int->name)
    ResolveEnumNumericValues();

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

void RETypeRegistry::ForEachProperty_BaseToDerived(const REType *type,
    const std::function<void(const REType &, const REProperty &)> &fn) const
{
    if (!type) return;

    // Collect chain
    std::vector<const REType*> chain;
    for (auto t = type; t != nullptr; t = GetBaseType(t))
        chain.push_back(t);

    // Reverse: base -> derived
    std::reverse(chain.begin(), chain.end());

    for (const REType* t : chain)
        for (const auto& p : t->properties)
            fn(*t, p);
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
            {
                std::cout << "    - " << v.name << (v.valueExpr.empty() ? "" : (" = " + v.valueExpr)) << "\n";
                for (const auto& m : v.meta)
                    std::cout << "        meta: " << m.key << (m.value.empty() ? "" : ("=" + m.value)) << "\n";
                std::cout << "    - " << v.name << " = " << v.valueI64 << (v.valueExpr.empty() ? "" :
                    ("  [expr: " + v.valueExpr + "]")) << "\n";
            }
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
            {
                std::cout << "  - " << v.name << (v.valueExpr.empty() ? "" : (" = " + v.valueExpr)) << "\n";
                for (const auto& m : v.meta)
                    std::cout << "      meta: " << m.key << (m.value.empty() ? "" : ("=" + m.value)) << "\n";
            }

            std::cout << "\n";
        }
    }
}
//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "REMeta.h"

struct FObjectInitializer;
class JCoreObject;

// ---------------- Reflection structures ----------------

// What kind of reflected type this is (needed for correct serialization & tooling)
enum class RETypeKind : uint8_t
{
    Class,
    Struct,
    Enum
};

// How to interpret a property at runtime (auto-serialization + object refs + enums)
enum class REPropKind : uint8_t
{
    Unknown,
    Value,           // serialize via value IO (by typeName)
    ReflectedStruct, // recursively serialize using reflectedType
    Enum,            // serialize via enum info (enumType)
    ObjectPtr        // serialize object reference (UUID/type/etc) using objectType
};

// Optional hooks (future-proof; can be null in MVP)
using RESerializeFn   = void(*)(class JsonWriter&, const void* obj);
using REDeserializeFn = void(*)(const class JsonReader&, void* obj);

// Optional upcast helper (future-proof for safe base walking; can be null for normal single inheritance)
using REUpcastFn = const void*(*)(const void* mostDerived);

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

struct REProperty
{
    std::string name;

    // Raw token string from generator (e.g. "int", "FTransform", "EThing", "JActor*")
    std::string typeName;

    // Resolved classification (filled in Finalize())
    REPropKind kind = REPropKind::Unknown;

    // If kind == ReflectedStruct: points to the reflected struct type
    const struct REType* reflectedType = nullptr;

    // If kind == Enum: points to enum info
    const REEnum* enumType = nullptr;

    // If kind == ObjectPtr: points to reflected class type of the pointee (best effort)
    const struct REType* objectType = nullptr;

    // Get pointer to the member from an object instance
    std::function<void*(void*)> getPtr;
    std::function<const void*(const void*)> getConstPtr;

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

struct REType
{
    std::string name;
    RETypeKind kind = RETypeKind::Class;

    std::type_index cppType{ typeid(void) };
    std::type_index baseCppType{ typeid(void) };

    // Optional safe base-walk helper (for now null is fine)
    REUpcastFn upcastFromMostDerived = nullptr;

    REMetaList meta; // class-level attributes from JCLASS/JSTRUCT

    std::vector<REProperty> properties;
    std::vector<REFunction> functions;

    // If this type represents an enum, link enum info here (filled in Finalize())
    const REEnum* enumInfo = nullptr;

    // Optional hooks (can stay null in MVP; serializer can use reflection data instead)
    RESerializeFn serializeFn = nullptr;
    REDeserializeFn deserializeFn = nullptr;

    std::function<JCoreObject*(const FObjectInitializer&)> factory;
};

class RETypeRegistry
{
public:
    // ---------------- Lifetime / access ----------------
    static RETypeRegistry& Get();

    // ---------------- Registration API (used by generated .refl.gen.cpp) ----------------

    // Register a type (class/struct/enum) with RTTI identity + base type
    // NOTE: for enums we can pass baseType = typeid(void)
    void BeginType(const char* name,
                   RETypeKind kind,
                   const std::type_info& selfType,
                   const std::type_info& baseType);

    // Optional safe base-walk helper (future-proof; can be omitted)
    void SetUpcast(const std::type_info& selfType, REUpcastFn upcastFn);

    // Add class-level meta (from JCLASS/JSTRUCT args)
    void AddTypeMeta(const std::type_info& ownerType,
                     const char* key,
                     const char* value = "");

    // Codegen path: register by pointer-to-member
    template<class TOwner, class TMember>
    void AddProperty(const std::type_info& ownerType,
                     const char* propName,
                     const char* propTypeName,
                     TMember TOwner::* memberPtr)
    {
        // Safety: ensure the registration call matches the template owner
        // (optional, but nice to catch accidental mismatches)
        const std::type_index idx(ownerType);
        const std::type_index expected(typeid(TOwner));
        // JASSERT(idx == expected);

        REType& T = EnsureTypeEntry(idx);

        REProperty P;
        P.name = propName;
        P.typeName = propTypeName;

        // Capture member pointer safely; works for private/protected when called in friend context
        P.getPtr = [memberPtr](void* obj) -> void* {
            auto* o = static_cast<TOwner*>(obj);
            return &(o->*memberPtr);
        };
        P.getConstPtr = [memberPtr](const void* obj) -> const void* {
            auto* o = static_cast<const TOwner*>(obj);
            return &(o->*memberPtr);
        };

        T.properties.emplace_back(std::move(P));
    }

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

    // Enum registration (name-driven; no RTTI required)
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
                std::function<JCoreObject*(const FObjectInitializer&)> factory);

    // Finalize type graph (resolve property kinds: struct/enum/object pointers)
    // Call once after static registration, early in engine startup.
    void Finalize();

    // ---------------- Lookup ----------------

    const REType* FindType(const std::type_info& ti) const;
    const REType* FindType(const std::type_index& idx) const;

    const REType* FindTypeByName(const std::string& name) const;

    const REEnum* FindEnumByName(const std::string& name) const;

    // Inheritance helpers
    const REType* GetBaseType(const REType* type) const;

    bool IsDerivedFrom(const REType* type, const REType* base) const;

    // Factory helper
    JCoreObject* CreateInstanceByTypeName(const std::string& name,
                                          const FObjectInitializer& Init) const;

    // Debug
    void DebugDumpAllTypes() const;

private:
    RETypeRegistry() = default;

    // internal helpers
    REType& EnsureTypeEntry(const std::type_index& idx);
    REType* FindTypeMutable(const std::type_index& idx);
    void ResolvePropertyKinds(REType& owner);

    // We store REType in unique_ptr to make returned const REType* stable forever.
    std::unordered_map<std::type_index, std::unique_ptr<REType>> m_Types;

    // Name->type pointer (stable because REType is heap-allocated)
    std::unordered_map<std::string, const REType*> m_NameToType;

    // Enums are name-driven (no RTTI necessarily)
    std::unordered_map<std::string, REEnum> m_Enums;
};
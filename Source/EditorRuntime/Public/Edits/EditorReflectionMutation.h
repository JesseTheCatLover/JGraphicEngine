//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "Core/Reflection/RETypeRegistry.h"
#include "Controllers/Inputs/FInspectorPanelInput.h"

class EditorRuntime;
class JCoreObject;
class JActor;
struct REProperty;

class EditorReflectionMutation
{
public:
    struct FKey
    {
        uint64_t contextActorID = 0;
        std::string objectUUID;
        std::string declaringTypeName;
        std::string propName;

        bool operator==(const FKey& o) const
        {
            return contextActorID == o.contextActorID
                && objectUUID == o.objectUUID
                && declaringTypeName == o.declaringTypeName
                && propName == o.propName;
        }
    };

    struct FKeyHash
    {
        size_t operator()(const FKey& k) const noexcept
        {
            size_t h = std::hash<uint64_t>{}(k.contextActorID);
            auto hc = [&](const std::string& s)
            {
                h ^= std::hash<std::string>{}(s) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
            };
            hc(k.objectUUID);
            hc(k.declaringTypeName);
            hc(k.propName);
            return h;
        }
    };

private:
    EditorRuntime& m_Runtime;

    // “Begin” snapshots for any reflected property edit session
    std::unordered_map<FKey, REVariant, FKeyHash> m_BeginValue;

public:
    explicit EditorReflectionMutation(EditorRuntime& runtime)
        : m_Runtime(runtime) {}

    bool ApplyReflectedEdit(const FInspectorEditCommand& cmd);

private:
    static REProperty* FindPropertyMutable(JCoreObject& obj,
                                           const std::string& declaringTypeName,
                                           const std::string& propName);

    static const void* ResolveDeclaringBasePtr_Const(const JCoreObject& mostDerived,
                                                     const std::string& declaringTypeName);
    static void* ResolveDeclaringBasePtr(JCoreObject& mostDerived,
                                         const std::string& declaringTypeName);

    static bool ReadVariantFromProperty(const REProperty& prop, const void* basePtr, REVariant& out);

    static bool VariantsEqual(const REVariant& a, const REVariant& b);
};
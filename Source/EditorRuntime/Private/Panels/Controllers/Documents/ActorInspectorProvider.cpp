//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Panels/Controllers/Documents/ActorInspectorProvider.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <cctype>
#include <string>
#include <functional>

#include "Panels/Controllers/Documents/FInspectorDocument.h"
#include "EditorCore/EditorHost.h"
#include "EditorCore/Services/SceneQueryService.h"

#include "Core/JCoreObject.h"
#include "Scene/JActor.h"

#include "Core/Reflection/REMeta.h"
#include "Core/Reflection/RETypeRegistry.h"
#include "EditorCore/Services/EditTimelineService.h"
#include "EditorEdits/UndoableActions/RenameActorAction.h"
#include "EditorEdits/UndoableActions/SetActorTransformAction.h"
#include "EditorEdits/UndoableActions/SetReflectedPropertyAction.h"

// ------------------------- helpers -------------------------

struct FInspectorContext
{
    bool bSchemaView = false;
};

namespace
{
    static bool IsUpper(char c) { return std::isupper((unsigned char)c) != 0; }
    static bool IsLower(char c) { return std::islower((unsigned char)c) != 0; }
    static bool IsDigit(char c) { return std::isdigit((unsigned char)c) != 0; }

    static std::string TrimInspectorPrefixes(std::string s)
    {
        // m_Foo / s_Foo
        if (s.size() >= 2 && (s[0] == 'm' || s[0] == 's') && s[1] == '_')
            s.erase(0, 2);

        // bIsActive (only if b + Uppercase)
        if (s.size() >= 2 && s[0] == 'b' && IsUpper(s[1]))
            s.erase(0, 1);

        // Optional: strip leading '_' leftovers
        while (!s.empty() && s[0] == '_')
            s.erase(0, 1);

        return s;
    }

    static std::string SplitIntoWords(const std::string& s)
    {
        std::string out;
        out.reserve(s.size() + 8);

        auto push_space_if_needed = [&]()
        {
            if (!out.empty() && out.back() != ' ')
                out.push_back(' ');
        };

        for (size_t i = 0; i < s.size(); ++i)
        {
            const char c = s[i];

            // underscores become spaces
            if (c == '_')
            {
                push_space_if_needed();
                continue;
            }

            const char prev = (i > 0) ? s[i - 1] : '\0';
            const char next = (i + 1 < s.size()) ? s[i + 1] : '\0';

            // Word boundary cases:
            // 1) lower -> Upper  (worldSpace => world Space)
            // 2) digit -> alpha or alpha -> digit (Foo2Bar => Foo 2 Bar)
            // 3) acronym boundary: "HTTPServer" => "HTTP Server"
            const bool boundary =
                (i > 0 && IsLower(prev) && IsUpper(c)) ||
                (i > 0 && IsDigit(prev) && !IsDigit(c)) ||
                (i > 0 && !IsDigit(prev) && IsDigit(c)) ||
                (i > 0 && IsUpper(prev) && IsUpper(c) && IsLower(next));

            if (boundary)
                push_space_if_needed();

            out.push_back(c);
        }

        // collapse multiple spaces (if any)
        std::string cleaned;
        cleaned.reserve(out.size());
        bool lastSpace = false;
        for (char c : out)
        {
            if (c == ' ')
            {
                if (!lastSpace) cleaned.push_back(c);
                lastSpace = true;
            }
            else
            {
                cleaned.push_back(c);
                lastSpace = false;
            }
        }

        // trim trailing space
        while (!cleaned.empty() && cleaned.back() == ' ')
            cleaned.pop_back();

        return cleaned;
    }

    static std::string PrettifyNameForInspector(std::string raw)
    {
        raw = TrimInspectorPrefixes(std::move(raw));
        return SplitIntoWords(raw);
    }

    static std::string MakeInspectorComponentLabel(const JCoreObject& obj)
    {
        const char* typeName =
            (obj.GetREType() ? obj.GetREType()->name.c_str() : "<unknown>");

        const std::string instanceName = obj.GetObjectName();

        if (!instanceName.empty())
            return instanceName + " (" + typeName + ")";

        // fallback if unnamed
        return std::string(typeName) + " (" + typeName + ")";
    }

    static std::string GetCategoryOrDefault(const REProperty& prop, const char* fallback)
    {
        std::string cat;
        if (REMetaSchema::Get().GetString(prop.meta, REMetaID::Category, cat) && !cat.empty())
            return cat;
        return (fallback && *fallback) ? fallback : "Default";
    }

    static std::string GetDisplayNameOrDefault(const REProperty& prop)
    {
        std::string dn;
        if (REMetaSchema::Get().GetString(prop.meta, REMetaID::DisplayName, dn) && !dn.empty())
            return dn;

        return PrettifyNameForInspector(prop.name);
    }

    static bool ShouldShowInInspector(const REProperty& prop, const FInspectorContext& ctx, bool& outReadOnly)
    {
        const REPropertyMetaResolved& m = prop.GetResolvedMeta();

        if (m.bHiddenInInspector)
            return false;

        if (ctx.bSchemaView)
        {
            if (m.editorScope == REEditorScope::InstanceOnly)
                return false;
        }
        else
        {
            if (m.editorScope == REEditorScope::SchemaOnly)
                return false;
        }

        outReadOnly = (m.editorVis == REEditorVis::Visible);
        return true;
    }

    static EInspectorWidget WidgetFromProperty(const REProperty& prop)
    {
        if (prop.kind == REPropKind::Enum)      return EInspectorWidget::Enum;
        if (prop.kind == REPropKind::ObjectPtr) return EInspectorWidget::ObjectRef;
        if (prop.kind == REPropKind::ReflectedStruct) return EInspectorWidget::Label; // MVP: collapsed struct
        if (prop.kind == REPropKind::Array) return EInspectorWidget::GenericArray;

        const std::string& tn = prop.typeName;

        if (tn == "bool") return EInspectorWidget::Bool;

        if (tn == "int" || tn == "int32" || tn == "size_t") return EInspectorWidget::Int;
        if (tn == "int64") return EInspectorWidget::Int;

        if (tn == "float") return EInspectorWidget::Float;
        if (tn == "double") return EInspectorWidget::Double;

        if (tn == "std::string") return EInspectorWidget::String;

        if (tn == "FVector2") return EInspectorWidget::Vec2;
        if (tn == "FVector3") return EInspectorWidget::Vec3;
        if (tn == "FVector4") return EInspectorWidget::Vec4;

        if (tn == "FQuat") return EInspectorWidget::Quat;
        if (tn == "FTransform") return EInspectorWidget::Transform;

        return EInspectorWidget::Label;
    }

    static std::string VariantToText(const REVariant& v)
    {
        switch (v.tag)
        {
            case REValueTag::Bool: return v.b ? "true" : "false";
            case REValueTag::Int: return std::to_string(v.i32);
            case REValueTag::Int64: return std::to_string(v.i64);
            case REValueTag::Float: return std::to_string(v.f32);
            case REValueTag::Double: return std::to_string(v.f64);
            case REValueTag::String: return v.s;
            case REValueTag::EnumInt64: return std::to_string(v.i64);
            case REValueTag::ObjectUUID: return v.s.empty() ? "<null>" : v.s;
            default: return "<unsupported>";
        }
    }

    static uint64_t Hash64_FNV1a(const void* data, size_t len)
    {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
        uint64_t h = 1469598103934665603ull;
        for (size_t i = 0; i < len; ++i)
        {
            h ^= (uint64_t)p[i];
            h *= 1099511628211ull;
        }
        return h;
    }

    static uint64_t HashString64(const std::string& s)
    {
        return Hash64_FNV1a(s.data(), s.size());
    }

    static uint64_t MakerowID(const std::string& objectUUID, const std::string& declaring, const std::string& prop)
    {
        // stable enough for MVP
        return HashString64(objectUUID + "::" + declaring + "::" + prop);
    }

    static uint64_t MakeTransformKey(uint64_t actorID, const char* prop)
    {
        // cheap stable hash
        uint64_t h = actorID;
        for (const char* p = prop; *p; ++p) h = h * 131 + (uint8_t)*p;
        return h;
    }

    static bool VariantsEqual(const REVariant& a, const REVariant& b)
{
    if (a.tag != b.tag) return false;

    switch (a.tag)
    {
        case REValueTag::Bool:      return a.b == b.b;
        case REValueTag::Int:       return a.i32 == b.i32;
        case REValueTag::Int64:     return a.i64 == b.i64;
        case REValueTag::Float:     return a.f32 == b.f32;   // exact is fine for undo (records real values)
        case REValueTag::Double:    return a.f64 == b.f64;
        case REValueTag::String:    return a.s == b.s;

        case REValueTag::Vec2:      return a.v2 == b.v2;
        case REValueTag::Vec3:      return a.v3 == b.v3;
        case REValueTag::Vec4:      return a.v4 == b.v4;

        case REValueTag::Quat:      return a.q == b.q;
        case REValueTag::Transform: return a.t == b.t;

        case REValueTag::EnumInt64: return a.i64 == b.i64;
        case REValueTag::ObjectUUID: return a.s == b.s;

        case REValueTag::VariantArray:
        {
            if (a.arrayElements.size() != b.arrayElements.size()) return false;
            for (size_t i = 0; i < a.arrayElements.size(); ++i) {
                if (!VariantsEqual(a.arrayElements[i], b.arrayElements[i])) return false;
            }
            return true;
        }

        default: return true;
    }
}

    // Finds the REType that matches declaringTypeName in the inheritance chain,
    // returns:
    //  - outDeclaringType: the REType* that owns the property
    //  - outDeclaringBasePtr: pointer to the correct base subobject for that declaring type
    //  - outProp: the REProperty* inside that declaring type
    static bool FindDeclaringTypeAndProperty(
        RETypeRegistry &reg,
        JCoreObject &obj,
        const std::string &declaringTypeName,
        const std::string &propName,
        const REType *&outDeclaringType,
        const void *&outDeclaringBasePtrConst,
        void *&outDeclaringBasePtr,
        REProperty *&outProp)
    {
        outDeclaringType = nullptr;
        outDeclaringBasePtrConst = nullptr;
        outDeclaringBasePtr = nullptr;
        outProp = nullptr;

        const REType *most = obj.GetREType();
        if (!most) return false;

        for (const REType *t = most; t != nullptr; t = reg.GetBaseType(t))
        {
            if (t->name != declaringTypeName)
                continue;

            // Compute base subobject pointer for this declaring type
            const void *baseConst = &obj;
            void *base = &obj;

            if (t != most && t->upcastFromMostDerived)
            {
                baseConst = t->upcastFromMostDerived(&obj);
                base = const_cast<void *>(baseConst);
            }

            // Find property on this declaring type
            auto *tm = const_cast<REType *>(t);
            for (auto &p: tm->properties)
            {
                if (p.name == propName)
                {
                    outDeclaringType = t;
                    outDeclaringBasePtrConst = baseConst;
                    outDeclaringBasePtr = base;
                    outProp = &p;
                    return true;
                }
            }

            return false;
        }

        return false;
    }

    static uint64_t MakePropEditKey(const std::string &objectUUID,
                                    const std::string &declaringTypeName,
                                    const std::string &propName)
    {
        return HashString64(objectUUID + "::" + declaringTypeName + "::" + propName);
    }

    static JCoreObject *FindTargetObjectByUUID(JActor &actor, const std::string &uuid)
    {
        if (actor.GetUUID() == uuid) return &actor;

        JSceneComponent *root = actor.GetRootComponent();
        if (root && root->GetUUID() == uuid) return root;

        for (JSceneComponent *sc: actor.GetSceneComponents())
            if (sc && sc->GetUUID() == uuid)
                return sc;

        for (JActorComponent* ac : actor.GetActorComponents())
            if (ac && ac->GetUUID() == uuid)
                return ac;

        return nullptr;
    }

    static REProperty* FindPropertyMutable(RETypeRegistry& reg, JCoreObject& obj, const std::string& declaringTypeName, const std::string& propName)
    {
        const REType* most = obj.GetREType();
        if (!most) return nullptr;

        for (const REType* t = most; t != nullptr; t = reg.GetBaseType(t))
        {
            if (t->name != declaringTypeName) continue;

            auto* tm = const_cast<REType*>(t);
            for (auto& p : tm->properties)
                if (p.name == propName)
                    return &p;
        }
        return nullptr;
    }

    static void BuildTargetFromObject(
        uint32_t providerID,
        uint64_t contextActorID,
        const JCoreObject& obj,
        const char* title,
        FInspectorTarget& outTarget)
    {
        outTarget.title = title ? title : "Object";
        outTarget.categories.clear();

        const REType* mostDerived = obj.GetREType();
        if (!mostDerived)
            return;

        FInspectorContext ctx;
        ctx.bSchemaView = obj.IsCDO();

        std::unordered_map<std::string, size_t> catIndex;
        catIndex.reserve(16);

        auto GetOrCreateCategory = [&](const std::string& name) -> FInspectorCategory&
        {
            auto it = catIndex.find(name);
            if (it != catIndex.end())
                return outTarget.categories[it->second];

            const size_t idx = outTarget.categories.size();
            outTarget.categories.push_back(FInspectorCategory{});
            auto& c = outTarget.categories.back();
            c.name = name;
            c.categoryID = HashString64(obj.GetUUID() + "::cat::" + name);
            catIndex[name] = idx;
            return c;
        };

        auto& reg = RETypeRegistry::Get();

        reg.ForEachProperty_BaseToDerived(mostDerived, [&](const REType& declaring, const REProperty& prop)
        {
            const void* instBase = &obj;
            if (&declaring != mostDerived && declaring.upcastFromMostDerived)
                instBase = declaring.upcastFromMostDerived(&obj);

            bool bReadOnly = false;
            if (!ShouldShowInInspector(prop, ctx, bReadOnly))
                return;

            const std::string category = GetCategoryOrDefault(prop, "Default");
            auto& cat = GetOrCreateCategory(category);

            FInspectorRow row;
            row.label = GetDisplayNameOrDefault(prop);
            row.rawName = prop.name;
            row.widget = WidgetFromProperty(prop);
            row.bReadOnly = bReadOnly;
            row.bMixed = false;
            row.meta = prop.GetResolvedMeta();

            row.enumInfo = (prop.kind == REPropKind::Enum) ? prop.enumType : nullptr;

            if (prop.kind == REPropKind::Enum)
            {
                row.enumSize   = prop.valueSize;  // we already set this in AddProperty()
                row.bEnumSigned = prop.bSigned;
            }

            RETypeRegistry::ReadVariantFromProperty(prop, instBase, row.value);

            // stable row id for UI state caching
            row.rowID = MakerowID(obj.GetUUID(), declaring.name, prop.name);

            // write routing
            row.write.contextRuntimeID = contextActorID;
            row.write.providerID = providerID;
            row.write.kind = EInspectorTargetKind::ObjectUUID;
            row.write.primaryID = obj.GetUUID();
            row.write.declaringTypeName = declaring.name;
            row.write.propName = prop.name;

            cat.rows.push_back(std::move(row));
        });
    }

    static void BuildActorTransformHeaderRows(
        uint32_t providerID,
        uint64_t contextActorID,
        JActor& actor,
        FInspectorTarget& actorTarget)
    {
        // Dedicated top "Essentials" rows for actor transform (not reflected category rows).
        // These are injected into a synthetic category and marked as Header so the panel
        // can draw them in a special block above normal categories.
        FInspectorCategory essentials;
        essentials.name = "__Essentials";
        essentials.categoryID = HashString64(actor.GetUUID() + "::cat::__Essentials");

        // Position
        {
            FInspectorRow row;
            row.rowID = HashString64(actor.GetUUID() + "::manual::__ActorPosition");
            row.label = "Position";
            row.rawName = "__ActorPosition";
            row.widget = EInspectorWidget::Vec3;
            row.presentation = EInspectorRowPresentation::Header;
            row.bReadOnly = false;
            row.bMixed = false;

            row.value = {};
            row.value.tag = REValueTag::Vec3;
            row.value.v3 = actor.GetActorLocation();

            row.write.contextRuntimeID = contextActorID;
            row.write.providerID = providerID;
            row.write.kind = EInspectorTargetKind::ObjectUUID;
            row.write.primaryID = actor.GetUUID();
            row.write.declaringTypeName = "__ManualActor";
            row.write.propName = "__ActorPosition";

            essentials.rows.push_back(std::move(row));
        }

        // Rotation (shown as Euler/Rotator vec3)
        {
            FInspectorRow row;
            row.rowID = HashString64(actor.GetUUID() + "::manual::__ActorRotation");
            row.label = "Rotation";
            row.rawName = "__ActorRotation";
            row.widget = EInspectorWidget::Vec3;
            row.presentation = EInspectorRowPresentation::Header;
            row.bReadOnly = false;
            row.bMixed = false;

            const FRotator r = actor.GetActorRotation();

            row.value = {};
            row.value.tag = REValueTag::Vec3;
            row.value.v3 = FVector3(r.pitch, r.yaw, r.roll);

            row.write.contextRuntimeID = contextActorID;
            row.write.providerID = providerID;
            row.write.kind = EInspectorTargetKind::ObjectUUID;
            row.write.primaryID = actor.GetUUID();
            row.write.declaringTypeName = "__ManualActor";
            row.write.propName = "__ActorRotation";

            essentials.rows.push_back(std::move(row));
        }

        // Scale
        {
            FInspectorRow row;
            row.rowID = HashString64(actor.GetUUID() + "::manual::__ActorScale");
            row.label = "Scale";
            row.rawName = "__ActorScale";
            row.widget = EInspectorWidget::Vec3;
            row.presentation = EInspectorRowPresentation::Header;
            row.bReadOnly = false;
            row.bMixed = false;

            row.value = {};
            row.value.tag = REValueTag::Vec3;
            row.value.v3 = actor.GetActorScale();

            row.write.contextRuntimeID = contextActorID;
            row.write.providerID = providerID;
            row.write.kind = EInspectorTargetKind::ObjectUUID;
            row.write.primaryID = actor.GetUUID();
            row.write.declaringTypeName = "__ManualActor";
            row.write.propName = "__ActorScale";

            essentials.rows.push_back(std::move(row));
        }

        actorTarget.categories.insert(actorTarget.categories.begin(), std::move(essentials));
    }

    static void BuildSceneTreeTargets(
        uint32_t providerID,
        uint64_t contextActorID,
        JActor& actor,
        std::vector<FInspectorTarget>& outTargets)
    {
        JSceneComponent* root = actor.GetRootComponent();
        const std::string rootUUID = root ? root->GetUUID() : std::string{};
        const auto& scenes = actor.GetSceneComponents();

        auto isRoot = [&](JSceneComponent* c) -> bool
        {
            if (!c) return false;
            if (!rootUUID.empty())
                return c->GetUUID() == rootUUID;
            return (root != nullptr) && (c == root);
        };


        // Map component ptr -> targetID (only for non-root scene components)
        std::unordered_map<JSceneComponent*, FRowID> idOf;
        idOf.reserve(scenes.size());

        for (JSceneComponent* sc : scenes)
        {
            if (!sc) continue;
            if (isRoot(sc)) continue; // robust root exclusion

            const FRowID tid = HashString64(std::string("scene:") + sc->GetUUID());
            idOf[sc] = tid;
        }

        // parentTargetID -> list of children scene components (ptrs)
        std::unordered_map<FRowID, std::vector<JSceneComponent*>> childrenByParentID;
        childrenByParentID.reserve(idOf.size());

        // Build adjacency
        for (auto& [sc, tid] : idOf)
        {
            JSceneComponent* parent = sc->GetParent();

            FRowID parentTID = 0;

            // If parent is root OR null OR not in visible set => top-level
            if (parent && !isRoot(parent))
            {
                auto it = idOf.find(parent);
                parentTID = (it != idOf.end()) ? it->second : 0;
            }

            childrenByParentID[parentTID].push_back(sc);
        }

        // DFS emit
        std::function<void(FRowID, uint32_t)> emit = [&](FRowID parentID, uint32_t depth)
        {
            auto it = childrenByParentID.find(parentID);
            if (it == childrenByParentID.end())
                return;

            for (JSceneComponent* sc : it->second)
            {
                if (!sc) continue;

                const FRowID myID = idOf[sc];
                JSceneComponent* parent = sc->GetParent();

                FRowID parentTargetID = 0;
                if (parent && parent != root)
                {
                    auto pit = idOf.find(parent);
                    if (pit != idOf.end())
                        parentTargetID = pit->second;
                }
                const std::string label = MakeInspectorComponentLabel(*sc);

                FInspectorTarget t;
                t.group = EInspectorTargetGroup::SceneComponent;
                t.targetID = myID;
                t.objectUUID = sc->GetUUID();
                t.parentTargetID = parentTargetID;
                t.depth = depth;

                t.listLabel = label;
                t.title     = label;

                BuildTargetFromObject(providerID, contextActorID, *sc, t.title.c_str(), t);
                outTargets.push_back(std::move(t));

                emit(myID, depth + 1);
            }
        };

        // Start from "root children" => parentTargetID = 0, depth=1
        emit(0, 1);
    }

    static void BuildActorComponentTargets(
        uint32_t providerID,
        uint64_t contextActorID,
        JActor& actor,
        std::vector<FInspectorTarget>& outTargets)
    {
        const auto& comps = actor.GetActorComponents();

        for (JActorComponent* c : comps)
        {
            if (!c) continue;

            const std::string label = MakeInspectorComponentLabel(*c);

            FInspectorTarget t;
            t.group = EInspectorTargetGroup::ActorComponent;
            t.targetID = HashString64(std::string("acomp:") + c->GetUUID());
            t.objectUUID = c->GetUUID();
            t.parentTargetID = 0;
            t.depth = 0;

            t.listLabel = label;
            t.title     = label;

            BuildTargetFromObject(providerID, contextActorID, *c, t.title.c_str(), t);
            outTargets.push_back(std::move(t));
        }
    }
}

// ------------------------- provider -------------------------

ActorInspectorProvider::ActorInspectorProvider(EditorHost& host)
    : m_Host(host)
{}

ActorInspectorProvider::~ActorInspectorProvider()
{
}

bool ActorInspectorProvider::CanHandle(const FInspectorSelection& sel) const
{
    return sel.runtimeID != 0;
}

void ActorInspectorProvider::BuildDocument(const FInspectorSelection& sel, FInspectorDocument& outDoc)
{
    outDoc.targets.clear();

    auto& queries = m_Host.GetService<SceneQueryService>();
    JActor* actor = queries.TryGetActor(sel.runtimeID);
    if (!actor)
        return;

    // 1) Actor target
    {
        FInspectorTarget t;
        t.group = EInspectorTargetGroup::Actor;
        t.targetID = HashString64(std::string("actor:") + actor->GetUUID());
        t.objectUUID = actor->GetUUID();

        t.listLabel = actor->GetActorName() + " (Instance)";
        t.title     = t.listLabel;

        BuildTargetFromObject(GetProviderID(), sel.runtimeID, *actor, t.title.c_str(), t);

        // Inject manual actor transform rows as top header rows
        BuildActorTransformHeaderRows(GetProviderID(), sel.runtimeID, *actor, t);

        outDoc.targets.push_back(std::move(t));
    }

    // 2) Scene components (tree, exclude root)
    BuildSceneTreeTargets(GetProviderID(), sel.runtimeID, *actor, outDoc.targets);

    // 3) Actor components (flat)
    BuildActorComponentTargets(GetProviderID(), sel.runtimeID, *actor, outDoc.targets);
}

void ActorInspectorProvider::ApplyEdit(const FInspectorEditCommand& cmd)
{
    if (cmd.handle.providerID != GetProviderID())
        return;
    if (cmd.handle.kind != EInspectorTargetKind::ObjectUUID)
        return;

    auto& queries  = m_Host.GetService<SceneQueryService>();
    auto& timeline = m_Host.GetService<EditTimelineService>();

    const uint64_t actorID = cmd.handle.contextRuntimeID;

    JActor* actor = queries.TryGetActor(actorID);
    if (!actor)
        return;

    // ------------------------------------------------------------
    // 1) Manual actor routing (Essentials)
    // ------------------------------------------------------------
    if (cmd.handle.primaryID == actor->GetUUID() &&
        cmd.handle.declaringTypeName == "__ManualActor")
    {
        // Position
        if (cmd.handle.propName == "__ActorPosition" && cmd.value.tag == REValueTag::Vec3)
        {
            const uint64_t key = MakeTransformKey(actorID, "__ActorPosition");

            if (cmd.phase == EInspectorEditPhase::Begin)
            {
                FTransform before;
                if (m_Host.GetRuntime().GetScene().TryGetActorWorldTransform(actorID, before))
                    m_TransformEditBegin[key] = before;
            }

            actor->SetActorLocation(cmd.value.v3);

            if (cmd.phase == EInspectorEditPhase::End)
            {
                auto it = m_TransformEditBegin.find(key);
                if (it == m_TransformEditBegin.end())
                    return;

                const FTransform before = it->second;
                m_TransformEditBegin.erase(it);

                FTransform after;
                if (!m_Host.GetRuntime().GetScene().TryGetActorWorldTransform(actorID, after))
                    return;

                if (after.GetPosition() != before.GetPosition())
                    timeline.Execute(MakeUnique<SetActorTransformAction>(m_Host.GetRuntime(), actorID, before, after));
            }
            return;
        }

        // Rotation
        if (cmd.handle.propName == "__ActorRotation" && cmd.value.tag == REValueTag::Vec3)
        {
            const uint64_t key = MakeTransformKey(actorID, "__ActorRotation");

            if (cmd.phase == EInspectorEditPhase::Begin)
            {
                FTransform before;
                if (m_Host.GetRuntime().GetScene().TryGetActorWorldTransform(actorID, before))
                    m_TransformEditBegin[key] = before;
            }

            actor->SetActorRotation(FRotator(cmd.value.v3.x, cmd.value.v3.y, cmd.value.v3.z));

            if (cmd.phase == EInspectorEditPhase::End)
            {
                auto it = m_TransformEditBegin.find(key);
                if (it == m_TransformEditBegin.end())
                    return;

                const FTransform before = it->second;
                m_TransformEditBegin.erase(it);

                FTransform after;
                if (!m_Host.GetRuntime().GetScene().TryGetActorWorldTransform(actorID, after))
                    return;

                if (after.GetRotation() != before.GetRotation())
                    timeline.Execute(MakeUnique<SetActorTransformAction>(m_Host.GetRuntime(), actorID, before, after));
            }
            return;
        }

        // Scale
        if (cmd.handle.propName == "__ActorScale" && cmd.value.tag == REValueTag::Vec3)
        {
            const uint64_t key = MakeTransformKey(actorID, "__ActorScale");

            if (cmd.phase == EInspectorEditPhase::Begin)
            {
                FTransform before;
                if (m_Host.GetRuntime().GetScene().TryGetActorWorldTransform(actorID, before))
                    m_TransformEditBegin[key] = before;
            }

            actor->SetActorScale(cmd.value.v3);

            if (cmd.phase == EInspectorEditPhase::End)
            {
                auto it = m_TransformEditBegin.find(key);
                if (it == m_TransformEditBegin.end())
                    return;

                const FTransform before = it->second;
                m_TransformEditBegin.erase(it);

                FTransform after;
                if (!m_Host.GetRuntime().GetScene().TryGetActorWorldTransform(actorID, after))
                    return;

                if (after.GetScale() != before.GetScale())
                    timeline.Execute(MakeUnique<SetActorTransformAction>(m_Host.GetRuntime(), actorID, before, after));
            }
            return;
        }

        // Name (commit style -> only on End)
        if (cmd.handle.propName == "__ActorName" && cmd.value.tag == REValueTag::String)
        {
            if (cmd.phase != EInspectorEditPhase::End)
                return;

            std::string newName = cmd.value.s;
            auto is_ws = [](unsigned char c) { return std::isspace(c) != 0; };
            while (!newName.empty() && is_ws((unsigned char)newName.front())) newName.erase(newName.begin());
            while (!newName.empty() && is_ws((unsigned char)newName.back()))  newName.pop_back();
            if (newName.empty())
                return;

            const std::string oldName = m_Host.GetRuntime().GetScene().GetActorName(actorID);
            if (oldName == newName)
                return;

            timeline.Execute(MakeUnique<RenameActorAction>(m_Host.GetRuntime(), actorID, oldName, newName));
            return;
        }

        return; // unknown manual prop -> ignore
    }

    // ------------------------------------------------------------
    // 2) Reflected property routing (undo via Begin/End snapshots)
    // ------------------------------------------------------------
    auto& scene = m_Host.GetRuntime().GetScene();

    const uint64_t key = MakePropEditKey(
        cmd.handle.primaryID,
        cmd.handle.declaringTypeName,
        cmd.handle.propName
    );

    // BEGIN snapshot (normal drag workflow)
    if (cmd.phase == EInspectorEditPhase::Begin)
    {
        REVariant before{};
        if (scene.TryReadReflectedProperty(actorID, cmd.handle.primaryID,
                                           cmd.handle.declaringTypeName, cmd.handle.propName,
                                           before))
        {
            m_PropEditBegin[key] = before;
        }

        // Live apply on Begin too
        scene.TryWriteReflectedProperty(actorID, cmd.handle.primaryID,
                                        cmd.handle.declaringTypeName, cmd.handle.propName,
                                        cmd.value);
        return;
}

// UPDATE: just apply
if (cmd.phase == EInspectorEditPhase::Update)
{
    scene.TryWriteReflectedProperty(actorID, cmd.handle.primaryID,
                                    cmd.handle.declaringTypeName, cmd.handle.propName,
                                    cmd.value);
    return;
}

// END: create undo (supports both drag and end-only edits)
if (cmd.phase == EInspectorEditPhase::End)
{
    REVariant before{};
    bool haveBefore = false;

    // If drag workflow, use stored Begin snapshot
    auto it = m_PropEditBegin.find(key);
    if (it != m_PropEditBegin.end())
    {
        before = it->second;
        haveBefore = true;
        m_PropEditBegin.erase(it);
    }
    else
    {
        // End-only workflow (checkbox, string commit): read before right now
        haveBefore = scene.TryReadReflectedProperty(actorID, cmd.handle.primaryID,
                                                    cmd.handle.declaringTypeName, cmd.handle.propName,
                                                    before);
    }

    // Apply final value
    scene.TryWriteReflectedProperty(actorID, cmd.handle.primaryID,
                                    cmd.handle.declaringTypeName, cmd.handle.propName,
                                    cmd.value);

    if (!haveBefore)
        return;

    REVariant after{};
    if (!scene.TryReadReflectedProperty(actorID, cmd.handle.primaryID,
                                        cmd.handle.declaringTypeName, cmd.handle.propName,
                                        after))
        return;

    if (VariantsEqual(before, after))
        return;

    timeline.Execute(MakeUnique<SetReflectedPropertyAction>(
        m_Host.GetRuntime(),
        actorID,
        cmd.handle.primaryID,
        cmd.handle.declaringTypeName,
        cmd.handle.propName,
        before,
        after
    ));
    return;
}
}
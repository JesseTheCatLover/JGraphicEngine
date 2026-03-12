//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Edits/EditorReflectionMutation.h"

#include <algorithm>

#include "EditorRuntime.h"
#include "Scene/JActor.h"
#include "Core/JCoreObject.h"
#include "Core/Reflection/REMeta.h"

#include "Edits/UndoableActions/SetReflectedPropertyAction.h"

bool EditorReflectionMutation::ApplyReflectedEdit(const FInspectorEditCommand& cmd)
{
    if (cmd.handle.kind != EInspectorTargetKind::ObjectUUID)
        return false;

    const uint64_t actorID = cmd.handle.contextRuntimeID;

    // keep manual actor routing elsewhere
    if (cmd.handle.declaringTypeName == "__ManualActor")
        return false;

    // Resolve target object purely via runtime scene api (NO host)
    JCoreObject* target = m_Runtime.GetScene().TryResolveObjectByUUID(actorID, cmd.handle.primaryID);
    if (!target)
        return true;

    REProperty* prop = FindPropertyMutable(*target, cmd.handle.declaringTypeName, cmd.handle.propName);
    if (!prop)
        return true;

    const auto& rm = prop->GetResolvedMeta();
    if (rm.bHiddenInInspector) return true;
    if (rm.editorVis == REEditorVis::Visible) return true;

    const void* basePtrC = ResolveDeclaringBasePtr_Const(*target, cmd.handle.declaringTypeName);
    void*       basePtr  = ResolveDeclaringBasePtr(*target, cmd.handle.declaringTypeName);

    FKey key;
    key.contextActorID    = actorID;
    key.objectUUID        = cmd.handle.primaryID;
    key.declaringTypeName = cmd.handle.declaringTypeName;
    key.propName          = cmd.handle.propName;

    if (cmd.phase == EInspectorEditPhase::Begin)
    {
        REVariant before{};
        ReadVariantFromProperty(*prop, basePtrC, before);
        m_BeginValue[key] = before;
    }

    // live apply for Begin/Update/End
    ApplyVariantToProperty(*prop, basePtr, cmd.value);

    if (cmd.phase == EInspectorEditPhase::End)
    {
        auto it = m_BeginValue.find(key);
        if (it == m_BeginValue.end())
            return true;

        const REVariant before = it->second;
        m_BeginValue.erase(it);

        REVariant after{};
        ReadVariantFromProperty(*prop, basePtrC, after);

        if (VariantsEqual(before, after))
            return true;

        // Title
        std::string title;
        {
            std::string dn;
            if (REMetaSchema::Get().GetString(prop->meta, REMetaID::DisplayName, dn) && !dn.empty())
                title = "Edit " + dn;
            else
                title = "Edit " + cmd.handle.propName;
        }

        if (IEditActionSink* sink = m_Runtime.GetEditSink())
        {
            sink->Submit(MakeUnique<SetReflectedPropertyAction>(
                m_Runtime,
                actorID,
                key.objectUUID,
                key.declaringTypeName,
                key.propName,
                before,
                after,
                title
            ));
        }
    }

    return true;
}
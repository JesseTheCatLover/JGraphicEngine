//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <vector>

#include "Controllers/Inputs/FInspectorPanelInput.h"
#include "Core/Reflection/REMeta.h"
#include "Core/Reflection/RETypeRegistry.h"

using FRowID = uint64_t; // stable across frames

enum class EInspectorWidget : uint8_t
{
    Label,
    Bool,
    Int,
    Float,
    Double,
    String,
    Vec2, Vec3, Vec4,
    Quat,
    Transform,
    Enum,
    ObjectRef
};

enum class EInspectorRowPresentation : uint8_t
{
    Normal,
    Header // drawn in the top essentials strip
};

struct FInspectorRow
{
    FRowID rowID = 0;

    std::string label;                // display
    std::string rawName;              // reflected propName
    EInspectorWidget widget = EInspectorWidget::Label;
    EInspectorRowPresentation presentation = EInspectorRowPresentation::Normal;

    REVariant value;                  // current value snapshot (for drawing)
    bool bReadOnly = false;
    bool bMixed = false;              // future multi-edit

    const REEnum* enumInfo = nullptr; // for enum types

    // enum storage details (only meaningful when enumInfo != nullptr)
    uint8_t enumSize = 0;             // underlying byte-size (1/2/4/8)
    bool    bEnumSigned = true;        // signedness of underlying type

    REPropertyMetaResolved meta;      // resolved meta snapshot for UI rules
    FInspectorWriteHandle write;      // where edits go
};

struct FInspectorCategory
{
    FRowID categoryID = 0;
    std::string name;
    std::vector<FInspectorRow> rows;
};

enum class EInspectorTargetGroup : uint8_t
{
    Actor,
    SceneComponent,
    ActorComponent
};

struct FInspectorTarget
{
    FRowID targetID = 0;
    std::string objectUUID;         // for command routing of target-level actions (rename, duplicate, etc.)

    // For component list rendering
    EInspectorTargetGroup group = EInspectorTargetGroup::Actor;
    FRowID parentTargetID = 0;      // only for SceneComponent tree
    uint32_t depth = 0;             // only for SceneComponent tree
    std::string listLabel;          // what shows in the component list

    // For properties
    std::string title;              // title shown above properties (optional)
    std::vector<FInspectorCategory> categories;
};


struct FInspectorDocument
{
    std::vector<FInspectorTarget> targets;
};
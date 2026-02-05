//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <vector>

#include "Core/Reflection/REMeta.h"

struct FInspectorRow
{
    std::string displayName;
    std::string declaringTypeName;
    std::string typeName;
    std::string valueText;
    //FPropertyMetadata metadata;
};

struct FInspectorCategorySnapshot
{
    std::string name;
    std::vector<FInspectorRow> rows;
};

struct FInspectorObjectSnapshot
{
    std::string displayName;     // "Actor", "Component: Camera", etc.
    std::string objectTypeName;
    std::string objectUUID;

    std::vector<FInspectorCategorySnapshot> categories;
};

struct FInspectorSnapshot
{
    std::vector<FInspectorObjectSnapshot> objects; // [0]=Actor, [1..]=Components
};
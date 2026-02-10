//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "FPanelInputBase.h"
#include "Core/Reflection/RETypeRegistry.h"

struct FInspectorEditCommand
{
    // Which object block (Actor / Component) to modify
    std::string objectUUID;

    // Which exact property
    std::string declaringTypeName; // "JActor" / base class name etc
    std::string propName;          // raw reflected name (not display)

    // New value payload
    REVariant value;
};

struct FInspectorPanelInput : public FPanelInputBase
{
    std::vector<FInspectorEditCommand> edits;
};
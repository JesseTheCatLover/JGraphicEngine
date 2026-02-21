//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "FPanelInputBase.h"
#include "Core/Reflection/RETypeRegistry.h"
#include <cstdint>

enum class EInspectorTargetKind : uint8_t { ObjectUUID, AssetKey };

struct FInspectorWriteHandle
{
    uint32_t providerID = 0;              // routes to correct provider
    EInspectorTargetKind kind = EInspectorTargetKind::ObjectUUID;

    uint64_t contextRuntimeID = 0;
    std::string primaryID;                // uuid / asset key / entity id string
    std::string declaringTypeName;
    std::string propName;
    // Later: uint32_t propId; (hashed, stable)
};

struct FInspectorEditCommand
{
    FInspectorWriteHandle handle;
    REVariant value;
};

struct FInspectorPanelInput : public FPanelInputBase
{
    std::vector<FInspectorEditCommand> edits;
};
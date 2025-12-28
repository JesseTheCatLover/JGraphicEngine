//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>

#include "Core/IEditorService.h"
#include "Scene/FRaycast.h"

using ActorID = uint64_t;

struct FSelectionModifiers;
class CameraEditorTool;
class EditorHost;

class PickingService : public IEditorService
{
private:
    EditorHost& m_Host;

    static FRay BuildRay(const CameraEditorTool& cam,
                         float width, float height,
                         float x, float y);

public:
    explicit PickingService(EditorHost& host);

    ActorID PickActorAtViewportPos(const CameraEditorTool& cam,
                       float width, float height,
                       float x, float y);

    void ApplyPickSelection(ActorID actorId, const FSelectionModifiers& mods);
};

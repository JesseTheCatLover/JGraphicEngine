//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>

#include "EditorCore/IEditorService.h"
#include "EditorAPI/Scene/FRaycast.h"

using ActorID = uint64_t;

struct FSelectionModifiers;
class CameraEditorTool;
class EditorHost;

class ScenePickingService : public IEditorService
{
private:
    EditorHost& m_Host;

public:
    explicit ScenePickingService(EditorHost& host);

    [[nodiscard]] ActorID PickActorAtViewportPos(const CameraEditorTool& cam,
                       float width, float height,
                       float x, float y) const;
};

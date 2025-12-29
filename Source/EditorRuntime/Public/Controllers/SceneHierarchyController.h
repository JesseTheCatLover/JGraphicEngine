//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "PanelRegistry.h"

struct FHierarchyOutput;
struct FHierarchyPanelInput;
class EditorHost;

class SceneHierarchyController
{
    PanelID m_PanelID = 0;
    EditorHost& m_Host;

public:
    SceneHierarchyController(PanelID id, EditorHost& host);

    void Update(float deltaTime, const FHierarchyPanelInput& input, FHierarchyOutput& out);
    void OnPanelDestroyed() {}
};

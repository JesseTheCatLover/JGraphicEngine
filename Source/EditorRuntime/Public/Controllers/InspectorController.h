// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "PanelRegistry.h"
#include "Controllers/Outputs/FInspectorSnapshot.h"

struct FInspectorPanelInput;
struct FInspectorOutput;
class EditorHost;


class InspectorController
{
    PanelID m_PanelID = 0;
    EditorHost& m_Host;


    // Per-panel persistent snapshot storage
    FInspectorSnapshot m_Snapshot;

public:
    InspectorController(PanelID id, EditorHost& host);


    void Update(float deltaTime, const FInspectorPanelInput& input, FInspectorOutput& out);
    void OnPanelDestroyed() {}
};
// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Panels/PanelRegistry.h"
#include "Core/Memory/SmartPointers.h"
#include "Documents/FInspectorDocument.h"

class ActorInspectorProvider;
struct FInspectorPanelInput;
struct FInspectorOutput;
class EditorHost;

class InspectorController
{
    PanelID m_PanelID = 0;
    EditorHost& m_Host;

    TUniquePtr<ActorInspectorProvider> m_ActorProvider;
    FInspectorDocument m_Doc;

public:
    InspectorController(PanelID id, EditorHost& host);
    ~InspectorController();


    void Update(float deltaTime, const FInspectorPanelInput& input, FInspectorOutput& out);
    void OnPanelDestroyed() {}
};
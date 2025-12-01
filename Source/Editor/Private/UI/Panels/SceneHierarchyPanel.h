//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "UI/IEditorPanels.h"

class EditorContext;

class SceneHierarchyPanel : public IEditorPanel
{
public:
    const char* GetName() const override { return "Scene Hierarchy"; }

    void Draw(EditorContext& context, EditorCore& core) override;
};
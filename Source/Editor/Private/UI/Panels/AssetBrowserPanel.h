//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "UI/IEditorPanels.h"

class AssetBrowserPanel final : public IEditorPanel
{
public:
    [[nodiscard]] const char* GetName() const override { return "AssetBrowser###AssetBrowser"; }
    [[nodiscard]] const char* GetPanelKey() const override { return "AssetBrowser"; }
    [[nodiscard]] EPanelDockGroup GetDockGroup() const override { return EPanelDockGroup::Single; }

    void OnDestroy(EditorHost& host) override;
    void Draw(EditorHost& host) override;

    
};

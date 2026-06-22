//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <vector>

#include "Panels/PanelRegistry.h"
#include "EditorCore/Services/AssetBrowser/AssetBrowserViewController.h"

struct FAssetBrowserOutput;
struct FAssetBrowserPanelInput;
class EditorHost;
class EditorRuntime;

class AssetBrowserController
{
private:
    // --- Core editor context ---
    PanelID m_PanelID = 0;
    EditorHost& m_Host;
    EditorRuntime& m_Runtime;

    FDelegateHandle m_AssetsMutatedHandle;

    AssetBrowserViewController m_TreeViewController;
    AssetBrowserViewController m_ContentViewController;

    std::vector<std::string> m_PreviousHistory;
    std::vector<std::string> m_ForwardHistory;

    bool m_bInternalNavigation = false;

public:
    AssetBrowserController(PanelID id, EditorHost& host, EditorRuntime& runtime);
    ~AssetBrowserController();


    void Update(float deltaTime, const FAssetBrowserPanelInput& input, FAssetBrowserOutput& out);
    void OnPanelDestroyed();

private:
    void NavigateTo(const std::string& path);
    void NavigateBack();
    void NavigateForward();

    void ProcessNavigation(const FAssetBrowserPanelInput &input);
    void ProcessSearch(const FAssetBrowserPanelInput &input);
    void ProcessExpansion(const FAssetBrowserPanelInput &input);
    void ProcessSelection(const FAssetBrowserPanelInput &input);
    void ProcessOpen(const FAssetBrowserPanelInput &input);
    void ProcessMutations(const FAssetBrowserPanelInput &input, AssetBrowserService& service);

};

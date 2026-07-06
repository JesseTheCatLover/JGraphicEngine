//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "EditorRuntime.h"

#include "Core/EngineContext.h"
#include "Core/EngineGlobals.h"
#include "Core/JEngine.h"
#include "Core/Project/ProjectContext.h"
#include "Framework/PostProcessManager.h"
#include "EditorInput/EditorInputDefaults.h"
#include "InputSystem/InputSubsystem.h"
#include "InputSystem/MappingStyles/ActionAxis/ActionAxisConfig.h"
#include "InputSystem/MappingStyles/ActionAxis/ActionAxisStyle.h"
#include "InputSystem/MappingStyles/Composite/CompositeInputMappingStyle.h"
#include "InputSystem/MappingStyles/HotkeyChord/HotkeyChordStyle.h"
#include "InputSystem/MappingStyles/HotkeyChord/HotkeyPlatformUtils.h"
#include "InputSystem/MappingStyles/HotkeyChord/HotkeySerialization.h"
#include "Rendering/FRenderView.h"
#include "Rendering/IPlatformSurface.h"
#include "Rendering/RendererSubsystem.h"
#include "Resources/ResourceSubsystem.h"
#include "Utilities/UPath.h"

EditorRuntime::EditorRuntime()
    : m_Context(JEngine::Get().GetEngineContext())
    , m_SceneManager(*JEngine::Get().GetSceneManager())
    , m_Renderer(*JEngine::Get().GetRenderer())
    , m_PlatformSurface(*JEngine::Get().GetPlatformSurface())
    , m_VirtualPathMounter(JEngine::Get().GetVirtualPathMounter())
    , m_AssetManager(*JEngine::Get().GetAssetManager())
    , m_Resource(*JEngine::Get().GetResourceSubsystem())
    , m_InputSubsystem(*JEngine::Get().GetInputSubsystem())
    , m_SceneAPI(m_Context, m_SceneManager, *JEngine::Get().GetDebugDraw())
    , m_ViewportAPI(m_Context, m_Renderer)
    , m_SurfaceAPI(m_Context, m_PlatformSurface, *JEngine::Get().GetInputManager())
    , m_FileAPI(m_Context, m_Resource, m_VirtualPathMounter, m_AssetManager)
{
    // Editor takes over rendering, so don't render directly to platform surface
    m_Context.SetShouldRenderToPlatformSurface(false);

    if (!InstallEditorInputMappings())
    {
        std::cerr << "[EditorRuntime]: Failed to install editor input mappings.\n";
    }

    auto& chain = GetPostProcessManager()->EditChain(kEditorPostProfile); // TODO: LEGACY
    chain.clear();

    FPostPassDesc outline{};
    outline.name = "Outline";
    outline.bEnabled = true;

    // Optional defaults (can be overridden per-view via frame params)
    // Edge/occlusion tuning
    outline.params.floats["u_DepthEpsilon"] = 0.0005f;
    outline.params.floats["u_Thickness"]    = 1.5f;   // try 1..3

    // Defaults
    outline.params.floats["u_Occlusion"] = 0.0f;  // 1=hide behind walls, 0=x-ray outlines
    outline.params.floats["u_FillAlpha"] = 0.08f; // subtle fill inside selection

    // Outline color (linear space)
    outline.params.floats["u_OutlineR"] = 1.0f;
    outline.params.floats["u_OutlineG"] = 0.65f;
    outline.params.floats["u_OutlineB"] = 0.10f;
    outline.params.floats["u_OutlineA"] = 1.0f;

    // outline.params.floats["u_OutlineR"] = 0.18f;
    // outline.params.floats["u_OutlineG"] = 0.28f;
    // outline.params.floats["u_OutlineB"] = 0.40f;
    // outline.params.floats["u_OutlineA"] = 1.0f;



    chain.push_back(std::move(outline));

    FPostPassDesc fxaa{};
    fxaa.name = "FXAA";
    fxaa.bEnabled = true;

    // Optional tuning (sane defaults)
    fxaa.params.floats["u_FXAA_ReduceMin"] = 1.0f / 128.0f;
    fxaa.params.floats["u_FXAA_ReduceMul"] = 1.0f / 8.0f;
    fxaa.params.floats["u_FXAA_SpanMax"]   = 4.0f;

    chain.push_back(fxaa);
}

EditorRuntime::~EditorRuntime()
{

}

void EditorRuntime::RestartEditor(const std::string &targetProjectPath, const std::string& extraArgs)
{
    JEngine::Get().RestartEditor(targetProjectPath, extraArgs);
}

bool EditorRuntime::InstallEditorInputMappings()
{
    FActionAxisMap axisMap = BuildEditorAxisMap();
    FHotkeyMap hotkeyDefaults = BuildEditorDefaultHotkeys();

    const std::string hotkeyDefaultsPath = UPath::Join(
     GEngine->GetProjectContext()->GetEngineRoot(), "Configs", "Editor", "Settings", "EditorHotkeys.Default.json");

    const std::string hotkeyUserPath = UPath::Join(
     GEngine->GetProjectContext()->GetEngineRoot(), "Configs", "Editor", "Settings", "EditorHotkeys.User.json");

    // 1) Ensure default file exists (but never overwrite it every launch)
    {
        FHotkeyMap tmp;
        if (!LoadHotkeyMapFromFile(hotkeyDefaultsPath, tmp))
        {
            // File missing or invalid -> generate from hardcoded defaults
            SaveHotkeyMapToFile(hotkeyDefaults, hotkeyDefaultsPath);
        }
    }

    const EInputPlatform platform = DetectInputPlatform();
    auto hotkeyStyle = MakeUnique<HotkeyChordStyle>(hotkeyDefaults, platform);

    // 2) Load user overrides if present; otherwise create a user file seeded from defaults
    bool bUserFileExists = false;
    {
        FHotkeyOverrides overrides;
        if (LoadHotkeyOverridesFromFile(hotkeyUserPath, overrides))
        {
            hotkeyStyle->ApplyOverrides(overrides);
            bUserFileExists = true;
        }
    }

    if (!bUserFileExists)
    {
        // First run: seed user file with defaults so users can edit it
        // (This writes a full map, not just overrides, which is friendlier.)
        SaveHotkeyMapToFile(hotkeyDefaults, hotkeyDefaultsPath);
    }

    auto composite = MakeUnique<CompositeInputMappingStyle>();
    composite->AddStyle(MakeUnique<ActionAxisStyle>(axisMap));
    composite->AddStyle(std::move(hotkeyStyle));

    m_InputSubsystem.SetMappingStyle(std::move(composite));
    return true;
}

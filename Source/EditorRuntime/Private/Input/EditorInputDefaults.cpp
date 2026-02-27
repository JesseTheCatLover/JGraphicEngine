//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Input/EditorInputDefaults.h"

namespace
{
    // Tiny helper to reduce boilerplate for axis bindings
    FInputBinding MakeBinding(
        EInputDeviceType deviceType,
        EPhysicalInput input,
        float scale = 1.0f,
        int deviceIndex = 0,
        EAxisComponent axisComponent = EAxisComponent::X,
        float deadZone = 0.0f,
        bool invert = false)
    {
        FInputBinding b;
        b.deviceType = deviceType;
        b.deviceIndex = deviceIndex;
        b.input = input;
        b.axisComponent = axisComponent;
        b.scale = scale;
        b.deadZone = deadZone;
        b.invert = invert;
        return b;
    }

    FActionAxisSlot MakeBoolSlot(const std::string& name, std::initializer_list<FInputBinding> bindings)
    {
        FActionAxisSlot slot;
        slot.name = name;
        slot.type = EInputChannelType::Bool;
        slot.bindings.assign(bindings.begin(), bindings.end());
        return slot;
    }

    FActionAxisSlot MakeAxis1DSlot(const std::string& name, std::initializer_list<FInputBinding> bindings)
    {
        FActionAxisSlot slot;
        slot.name = name;
        slot.type = EInputChannelType::Axis1D;
        slot.bindings.assign(bindings.begin(), bindings.end());
        return slot;
    }

    FActionAxisSlot MakeAxis2DSlot(const std::string& name, std::initializer_list<FInputBinding> bindings)
    {
        FActionAxisSlot slot;
        slot.name = name;
        slot.type = EInputChannelType::Axis2D;
        slot.bindings.assign(bindings.begin(), bindings.end());
        return slot;
    }

    FHotkeyChord MakeChord(
        std::initializer_list<EPhysicalInput> keys,
        EHotkeyPlatformMask platforms,
        bool allowExtraModifiers = true,
        bool allowExtraKeys = false)
    {
        FHotkeyChord c;
        c.keys.assign(keys.begin(), keys.end());
        c.platforms = platforms;
        c.allowExtraModifiers = allowExtraModifiers;
        c.allowExtraKeys = allowExtraKeys;
        return c;
    }

    FHotkeyCommand MakeCommand(
        const std::string& name,
        const std::string& category,
        const std::string& description,
        std::initializer_list<FHotkeyChord> defaults)
    {
        FHotkeyCommand cmd;
        cmd.name = name;
        cmd.category = category;
        cmd.description = description;
        cmd.defaultChords.assign(defaults.begin(), defaults.end());
        cmd.chords = cmd.defaultChords;
        return cmd;
    }
}

FActionAxisMap BuildEditorAxisMap()
{
    FActionAxisMap map;

    // ------------------------------------------------
    // Editor Camera Movement (WASD + QE)
    // ------------------------------------------------

    // Forward/Back (W/S)
    map.actions.push_back(MakeAxis1DSlot("EditorCamera.MoveForward",
    {
        MakeBinding(EInputDeviceType::Keyboard, EPhysicalInput::Key_W, +1.0f),
        MakeBinding(EInputDeviceType::Keyboard, EPhysicalInput::Key_S, -1.0f)
    }));

    // Right/Left (D/A)
    map.actions.push_back(MakeAxis1DSlot("EditorCamera.MoveRight",
    {
        MakeBinding(EInputDeviceType::Keyboard, EPhysicalInput::Key_D, +1.0f),
        MakeBinding(EInputDeviceType::Keyboard, EPhysicalInput::Key_A, -1.0f)
    }));

    // Up/Down (Space/Shift)
    map.actions.push_back(MakeAxis1DSlot("EditorCamera.MoveUp",
    {
        MakeBinding(EInputDeviceType::Keyboard, EPhysicalInput::Key_Space, +1.0f),
        MakeBinding(EInputDeviceType::Keyboard, EPhysicalInput::Key_LeftShift, -1.0f)
    }));

    // ------------------------------------------------
    // Mouse look and pan
    // ------------------------------------------------

    // Look delta from raw mouse delta
    map.actions.push_back(MakeAxis2DSlot("EditorCamera.Look",
    {
        MakeBinding(EInputDeviceType::Mouse, EPhysicalInput::Mouse_DeltaX, 0.0038f, 0, EAxisComponent::X),
        MakeBinding(EInputDeviceType::Mouse, EPhysicalInput::Mouse_DeltaY, 0.0038f, 0, EAxisComponent::Y)
    }));

    // Pan delta (same source, gated in editor code by middle mouse held if desired)
    map.actions.push_back(MakeAxis2DSlot("EditorCamera.Pan",
    {
        MakeBinding(EInputDeviceType::Mouse, EPhysicalInput::Mouse_DeltaX, +1.0f, 0, EAxisComponent::X),
        MakeBinding(EInputDeviceType::Mouse, EPhysicalInput::Mouse_DeltaY, +1.0f, 0, EAxisComponent::Y)
    }));

    // Zoom from wheel Y
    map.actions.push_back(MakeAxis1DSlot("EditorCamera.Zoom",
    {
        MakeBinding(EInputDeviceType::Mouse, EPhysicalInput::Mouse_WheelY, +1.0f)
    }));

    // ------------------------------------------------
    // Mouse buttons (useful for viewport interaction state)
    // ------------------------------------------------
    map.actions.push_back(MakeBoolSlot("EditorViewport.LMB",
    {
        MakeBinding(EInputDeviceType::Mouse, EPhysicalInput::Mouse_ButtonLeft)
    }));

    map.actions.push_back(MakeBoolSlot("EditorViewport.RMB",
    {
        MakeBinding(EInputDeviceType::Mouse, EPhysicalInput::Mouse_ButtonRight)
    }));

    map.actions.push_back(MakeBoolSlot("EditorViewport.MMB",
    {
        MakeBinding(EInputDeviceType::Mouse, EPhysicalInput::Mouse_ButtonMiddle)
    }));

    // Alt
    map.actions.push_back(MakeBoolSlot("Editor.Modifier.Alt",
    {
        MakeBinding(EInputDeviceType::Keyboard, EPhysicalInput::Key_LeftAlt),
        MakeBinding(EInputDeviceType::Keyboard, EPhysicalInput::Key_RightAlt)
    }));

    // Shift
    map.actions.push_back(MakeBoolSlot("Editor.Modifier.Shift",
    {
        MakeBinding(EInputDeviceType::Keyboard, EPhysicalInput::Key_LeftShift),
        MakeBinding(EInputDeviceType::Keyboard, EPhysicalInput::Key_RightShift)
    }));

    return map;
}

FHotkeyMap BuildEditorDefaultHotkeys()
{
    FHotkeyMap map;

    const auto WL  = EHotkeyPlatformMask::Windows | EHotkeyPlatformMask::Linux;
    const auto MAC = EHotkeyPlatformMask::MacOS;
    const auto ANY = EHotkeyPlatformMask::Any;

    // ---------------- App ----------------
    map.commands.push_back(MakeCommand(
        "Editor.App.Quit",
        "Application",
        "Quit editor",
        {
            MakeChord({ EPhysicalInput::Key_Escape }, ANY)
        }));

    // ---------------- File ----------------
    map.commands.push_back(MakeCommand(
        "Editor.File.NewScene",
        "File",
        "Create a new scene",
        {
            MakeChord({ EPhysicalInput::Key_LeftControl, EPhysicalInput::Key_N }, WL),
            MakeChord({ EPhysicalInput::Key_LeftSuper,   EPhysicalInput::Key_N }, MAC)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.File.OpenScene",
        "File",
        "Open a scene",
        {
            MakeChord({ EPhysicalInput::Key_LeftControl, EPhysicalInput::Key_O }, WL),
            MakeChord({ EPhysicalInput::Key_LeftSuper,   EPhysicalInput::Key_O }, MAC)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.File.Save",
        "File",
        "Save current scene",
        {
            MakeChord({ EPhysicalInput::Key_LeftControl, EPhysicalInput::Key_S }, WL),
            MakeChord({ EPhysicalInput::Key_LeftSuper,   EPhysicalInput::Key_S }, MAC)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.File.SaveAs",
        "File",
        "Save current scene as",
        {
            MakeChord({ EPhysicalInput::Key_LeftControl, EPhysicalInput::Key_LeftAlt,   EPhysicalInput::Key_S }, WL),
            MakeChord({ EPhysicalInput::Key_LeftSuper,   EPhysicalInput::Key_LeftAlt,   EPhysicalInput::Key_S }, MAC)
            // If you prefer Shift instead of Alt on macOS, swap this later.
        }));

    map.commands.push_back(MakeCommand(
        "Editor.File.SaveAll",
        "File",
        "Save all modified content",
        {
            MakeChord({ EPhysicalInput::Key_LeftControl, EPhysicalInput::Key_LeftShift, EPhysicalInput::Key_S }, WL),
            MakeChord({ EPhysicalInput::Key_LeftSuper,   EPhysicalInput::Key_LeftShift, EPhysicalInput::Key_S }, MAC)
        }));

    // ---------------- Edit ----------------
    map.commands.push_back(MakeCommand(
        "Editor.Edit.Copy",
        "Edit",
        "Copy selected objects",
        {
            MakeChord({ EPhysicalInput::Key_LeftControl, EPhysicalInput::Key_C }, WL),
            MakeChord({ EPhysicalInput::Key_LeftSuper,   EPhysicalInput::Key_C }, MAC)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.Edit.Paste",
        "Edit",
        "Paste objects",
        {
            MakeChord({ EPhysicalInput::Key_LeftControl, EPhysicalInput::Key_V }, WL),
            MakeChord({ EPhysicalInput::Key_LeftSuper,   EPhysicalInput::Key_V }, MAC)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.Edit.Undo",
        "Edit",
        "Undo last action",
        {
            MakeChord({ EPhysicalInput::Key_LeftControl, EPhysicalInput::Key_Z }, WL),
            MakeChord({ EPhysicalInput::Key_LeftSuper,   EPhysicalInput::Key_Z }, MAC)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.Edit.Redo",
        "Edit",
        "Redo last action",
        {
            MakeChord({ EPhysicalInput::Key_LeftControl, EPhysicalInput::Key_Y }, WL),
            MakeChord({ EPhysicalInput::Key_LeftSuper,   EPhysicalInput::Key_LeftShift, EPhysicalInput::Key_Z }, MAC)
        }));

    // ---------------- View ----------------
    map.commands.push_back(MakeCommand(
        "Editor.View.ToggleSceneHierarchy",
        "View",
        "Toggle Scene Hierarchy panel",
        {
            MakeChord({ EPhysicalInput::Key_Z }, ANY)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.View.ToggleInspector",
        "View",
        "Toggle Inspector panel",
        {
            MakeChord({ EPhysicalInput::Key_V }, ANY)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.View.ToggleAssetBrowser",
        "View",
        "Toggle Asset Browser panel",
        {
            MakeChord({ EPhysicalInput::Key_C }, ANY)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.View.ToggleConsole",
        "View",
        "Toggle Console panel",
        {
            MakeChord({ EPhysicalInput::Key_X }, ANY)
        }));

    // ---------------- Viewport ----------------
    map.commands.push_back(MakeCommand(
        "Editor.Viewport.FocusSelection",
        "Viewport",
        "Focus camera on selection",
        {
            MakeChord({ EPhysicalInput::Key_F }, ANY)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.Viewport.ToggleGameView",
        "Viewport",
        "Toggle game view",
        {
            MakeChord({ EPhysicalInput::Key_G }, ANY)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.Viewport.MultiViewModeMenu",
        "Viewport",
        "Open or trigger multi-view mode controls",
        {
            MakeChord({ EPhysicalInput::Key_LeftControl, EPhysicalInput::Key_M, EPhysicalInput::Key_V }, WL),
            MakeChord({ EPhysicalInput::Key_LeftSuper,   EPhysicalInput::Key_M, EPhysicalInput::Key_V }, MAC)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.Viewport.SetSingleView",
        "Viewport",
        "Set viewport layout to single view",
        {
            MakeChord({ EPhysicalInput::Key_LeftControl, EPhysicalInput::Key_V, EPhysicalInput::Key_1 }, WL),
            MakeChord({ EPhysicalInput::Key_LeftSuper,   EPhysicalInput::Key_V, EPhysicalInput::Key_1 }, MAC)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.Viewport.SetDoubleView",
        "Viewport",
        "Set viewport layout to double view",
        {
            MakeChord({ EPhysicalInput::Key_LeftControl, EPhysicalInput::Key_V, EPhysicalInput::Key_2 }, WL),
            MakeChord({ EPhysicalInput::Key_LeftSuper,   EPhysicalInput::Key_V, EPhysicalInput::Key_2 }, MAC)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.Viewport.SetTripleView",
        "Viewport",
        "Set viewport layout to triple view",
        {
            MakeChord({ EPhysicalInput::Key_LeftControl, EPhysicalInput::Key_V, EPhysicalInput::Key_3 }, WL),
            MakeChord({ EPhysicalInput::Key_LeftSuper,   EPhysicalInput::Key_V, EPhysicalInput::Key_3 }, MAC)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.Viewport.SetQuadView",
        "Viewport",
        "Set viewport layout to quad view",
        {
            MakeChord({ EPhysicalInput::Key_LeftControl, EPhysicalInput::Key_V, EPhysicalInput::Key_4 }, WL),
            MakeChord({ EPhysicalInput::Key_LeftSuper,   EPhysicalInput::Key_V, EPhysicalInput::Key_4 }, MAC)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.Viewport.ToggleTabVisibility",
        "Viewport",
        "Toggle viewport dock tab visibility",
        {
            MakeChord({ EPhysicalInput::Key_LeftControl, EPhysicalInput::Key_V, EPhysicalInput::Key_H }, WL),
            MakeChord({ EPhysicalInput::Key_LeftSuper,   EPhysicalInput::Key_V, EPhysicalInput::Key_H }, MAC)
        }));

    // ---------------- Tools ----------------
    map.commands.push_back(MakeCommand(
        "Editor.Tools.Translate",
        "Tools",
        "Select translate tool",
        {
            MakeChord({ EPhysicalInput::Key_W }, ANY)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.Tools.Rotate",
        "Tools",
        "Select rotate tool",
        {
            MakeChord({ EPhysicalInput::Key_R }, ANY)
        }));

    map.commands.push_back(MakeCommand(
        "Editor.Tools.Scale",
        "Tools",
        "Select scale tool",
        {
            MakeChord({ EPhysicalInput::Key_E }, ANY)
        }));

    return map;
}
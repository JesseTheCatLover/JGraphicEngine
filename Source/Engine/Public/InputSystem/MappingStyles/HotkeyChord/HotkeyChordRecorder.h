//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>

#include "HotkeyChordConfig.h"
#include "InputSystem/FInputDeviceState.h"

class HotkeyChordRecorder
{
public:
    void BeginCapture();
    void CancelCapture();

    [[nodiscard]] bool IsCapturing() const { return m_IsCapturing; }
    [[nodiscard]] bool HasCapturedChord() const { return m_HasCaptured; }

    // Feed every frame while UI is listening
    void Update(
        const std::vector<FInputDeviceState>& devices,
        const std::vector<FInputDeviceState>& prevDevices);

    // Valid only if HasCapturedChord() == true
    FHotkeyChord ConsumeCapturedChord();

    // Optional policy
    void SetAllowSingleKey(bool allow) { m_AllowSingleKey = allow; }
    void SetAllowNonKeyboard(bool allow) { m_AllowNonKeyboard = allow; } // reserved for future

private:
    bool m_IsCapturing = false;
    bool m_HasCaptured = false;
    bool m_HasSeenAnyPressed = false;
    bool m_AllowSingleKey = true;
    bool m_AllowNonKeyboard = false; // currently unused

    FHotkeyChord m_WorkingChord;
    FHotkeyChord m_CapturedChord;

private:
    static bool IsModifierKey(EPhysicalInput key);
    static int KeySortPriority(EPhysicalInput key);
    static void NormalizeChord(FHotkeyChord& chord, bool bCanonicalize);

    [[nodiscard]] const FInputDeviceState* FindKeyboard(const std::vector<FInputDeviceState>& devices) const;
    [[nodiscard]] bool IsKeyDown(const FInputDeviceState* keyboard, EPhysicalInput key) const;

    void CollectCurrentlyHeldKeys(const std::vector<FInputDeviceState>& devices);
};
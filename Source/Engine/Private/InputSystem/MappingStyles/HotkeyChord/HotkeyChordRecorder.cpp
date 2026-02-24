//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "InputSystem/MappingStyles/HotkeyChord/HotkeyChordRecorder.h"

#include <algorithm>

namespace
{
    static EPhysicalInput CanonicalizeModifier(EPhysicalInput k)
    {
        switch (k)
        {
            case EPhysicalInput::Key_RightControl: return EPhysicalInput::Key_LeftControl;
            case EPhysicalInput::Key_RightShift:   return EPhysicalInput::Key_LeftShift;
            case EPhysicalInput::Key_RightAlt:     return EPhysicalInput::Key_LeftAlt;
            case EPhysicalInput::Key_RightSuper:   return EPhysicalInput::Key_LeftSuper;
            default:                               return k;
        }
    }
}

void HotkeyChordRecorder::BeginCapture()
{
    m_IsCapturing = true;
    m_HasCaptured = false;
    m_HasSeenAnyPressed = false;
    m_WorkingChord = {};
    m_CapturedChord = {};
}

void HotkeyChordRecorder::CancelCapture()
{
    m_IsCapturing = false;
    m_HasCaptured = false;
    m_HasSeenAnyPressed = false;
    m_WorkingChord = {};
    m_CapturedChord = {};
}

void HotkeyChordRecorder::Update(
    const std::vector<FInputDeviceState>& devices,
    const std::vector<FInputDeviceState>& prevDevices)
{
    (void)prevDevices; // not strictly needed with current logic, but handy for future refinements

    if (!m_IsCapturing)
        return;

    const FInputDeviceState* kb = FindKeyboard(devices);
    if (!kb)
        return;

    bool anyDown = false;
    for (const auto& kv : kb->values)
    {
        if (!IsKeyboard(kv.first))
            continue;
        if (kv.second > 0.5f)
        {
            anyDown = true;
            break;
        }
    }

    // If any keyboard key is currently down, keep collecting
    if (anyDown)
    {
        m_HasSeenAnyPressed = true;
        CollectCurrentlyHeldKeys(devices);
        return;
    }

    // If no keys are down and we had a chord in progress, finalize it
    if (m_HasSeenAnyPressed)
    {
        NormalizeChord(m_WorkingChord);

        // Optionally reject empty/single key chords
        if (!m_WorkingChord.keys.empty())
        {
            if (m_AllowSingleKey || m_WorkingChord.keys.size() > 1)
            {
                m_CapturedChord = m_WorkingChord;
                m_HasCaptured = true;
            }
        }

        m_IsCapturing = false;
        m_HasSeenAnyPressed = false;
        m_WorkingChord = {};
    }
}

FHotkeyChord HotkeyChordRecorder::ConsumeCapturedChord()
{
    m_HasCaptured = false;
    return m_CapturedChord;
}

bool HotkeyChordRecorder::IsModifierKey(EPhysicalInput key)
{
    switch (key)
    {
    case EPhysicalInput::Key_LeftControl:
    case EPhysicalInput::Key_RightControl:
    case EPhysicalInput::Key_LeftShift:
    case EPhysicalInput::Key_RightShift:
    case EPhysicalInput::Key_LeftAlt:
    case EPhysicalInput::Key_RightAlt:
    case EPhysicalInput::Key_LeftSuper:
    case EPhysicalInput::Key_RightSuper:
        return true;
    default:
        return false;
    }
}

int HotkeyChordRecorder::KeySortPriority(EPhysicalInput key)
{
    switch (key)
    {
    case EPhysicalInput::Key_LeftControl:
    case EPhysicalInput::Key_RightControl: return 0;
    case EPhysicalInput::Key_LeftShift:
    case EPhysicalInput::Key_RightShift:   return 1;
    case EPhysicalInput::Key_LeftAlt:
    case EPhysicalInput::Key_RightAlt:     return 2;
    case EPhysicalInput::Key_LeftSuper:
    case EPhysicalInput::Key_RightSuper:   return 3;
    default:                               return 10;
    }
}

void HotkeyChordRecorder::NormalizeChord(FHotkeyChord& chord, bool bCanonicalize)
{
    if (bCanonicalize)
    {
        for (EPhysicalInput& k : chord.keys)
        {
            k = CanonicalizeModifier(k);
        }
    }

    chord.keys.erase(
        std::remove(chord.keys.begin(), chord.keys.end(), EPhysicalInput::Unknown),
        chord.keys.end());

    std::sort(chord.keys.begin(), chord.keys.end(),
        [](EPhysicalInput a, EPhysicalInput b)
        {
            const int pa = KeySortPriority(a);
            const int pb = KeySortPriority(b);
            if (pa != pb) return pa < pb;
            return static_cast<uint16_t>(a) < static_cast<uint16_t>(b);
        });

    chord.keys.erase(std::unique(chord.keys.begin(), chord.keys.end()), chord.keys.end());
}

const FInputDeviceState* HotkeyChordRecorder::FindKeyboard(const std::vector<FInputDeviceState>& devices) const
{
    for (const FInputDeviceState& d : devices)
    {
        if (d.type == EInputDeviceType::Keyboard)
            return &d;
    }
    return nullptr;
}

bool HotkeyChordRecorder::IsKeyDown(const FInputDeviceState* keyboard, EPhysicalInput key) const
{
    if (!keyboard)
        return false;

    auto it = keyboard->values.find(key);
    if (it == keyboard->values.end())
        return false;

    return it->second > 0.5f;
}

void HotkeyChordRecorder::CollectCurrentlyHeldKeys(const std::vector<FInputDeviceState>& devices)
{
    const FInputDeviceState* kb = FindKeyboard(devices);
    if (!kb)
        return;

    // Store union of all keys observed during this capture session
    for (const auto& kv : kb->values)
    {
        const EPhysicalInput key = kv.first;
        const float value = kv.second;

        if (!IsKeyboard(key))
            continue;
        if (value <= 0.5f)
            continue;

        if (std::find(m_WorkingChord.keys.begin(), m_WorkingChord.keys.end(), key) == m_WorkingChord.keys.end())
        {
            m_WorkingChord.keys.push_back(key);
        }
    }
}
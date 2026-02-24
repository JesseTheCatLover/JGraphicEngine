//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "InputSystem/MappingStyles/HotkeyChord/HotkeyChordStyle.h"
#include "InputSystem/MappingStyles/HotkeyChord/FHotkeyConflict.h"

#include <algorithm>
#include <cmath>

namespace
{
    static bool IsModifierKey(EPhysicalInput k)
    {
        switch (k)
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

    static int KeySortPriority(EPhysicalInput k)
    {
        // Modifiers first in a stable readable order
        switch (k)
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

    static bool ContainsKey(const std::vector<EPhysicalInput>& keys, EPhysicalInput k)
    {
        return std::find(keys.begin(), keys.end(), k) != keys.end();
    }

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

HotkeyChordStyle::HotkeyChordStyle(FHotkeyMap map, EInputPlatform platform)
    : m_Map(std::move(map))
    , m_Platform(platform)
{
    // Normalize all chords once for stable compare/display/save
    for (FHotkeyCommand& cmd : m_Map.commands)
    {
        for (FHotkeyChord& c : cmd.chords)
            NormalizeChord(c);
        for (FHotkeyChord& c : cmd.defaultChords)
            NormalizeChord(c);
    }
}

void HotkeyChordStyle::BuildChannels(std::vector<FInputChannelDesc>& outChannels)
{
    outChannels.clear();
    m_NameToHandle.clear();

    outChannels.reserve(m_Map.commands.size());

    for (size_t i = 0; i < m_Map.commands.size(); ++i)
    {
        const FHotkeyCommand& cmd = m_Map.commands[i];

        FInputChannelDesc desc;
        desc.handle = static_cast<InputChannelHandle>(i);
        desc.name   = cmd.name;
        desc.type   = EInputChannelType::Bool;

        outChannels.push_back(desc);
        m_NameToHandle[cmd.name] = desc.handle;
    }

    m_BoolStates.clear();
    m_BoolStates.resize(outChannels.size());
}

void HotkeyChordStyle::UpdateChannels(
    float dt,
    const std::vector<FInputDeviceState>& devices,
    const std::vector<FInputDeviceState>& prevDevices,
    std::vector<float>& channelData)
{
    (void)dt;
    (void)channelData;

    for (size_t i = 0; i < m_Map.commands.size(); ++i)
    {
        const FHotkeyCommand& cmd = m_Map.commands[i];
        FActionStateBool& st = m_BoolStates[i];

        const bool wasHeld = st.held;

        bool heldNow = false;
        bool pressedNow = false;

        // OR semantics across alternates
        for (const FHotkeyChord& chord : cmd.chords)
        {
            if (!ChordAppliesToCurrentPlatform(chord))
                continue;

            if (!IsChordHeld(chord, devices))
                continue;

            heldNow = true;

            if (HasNewPressInChord(chord, devices, prevDevices))
            {
                pressedNow = true;
                // no break: held already true; pressed true is enough
            }
        }

        st.held = heldNow;
        st.pressed = pressedNow;
        st.released = (!heldNow && wasHeld);
    }
}

FActionStateBool HotkeyChordStyle::GetBoolState(InputChannelHandle handle) const
{
    if (handle >= m_BoolStates.size())
        return {};
    return m_BoolStates[handle];
}

FActionStateAxis1D HotkeyChordStyle::GetAxis1DState(InputChannelHandle handle) const
{
    (void)handle;
    return {};
}

FActionStateAxis2D HotkeyChordStyle::GetAxis2DState(InputChannelHandle handle) const
{
    (void)handle;
    return {};
}

// ---------------- Runtime Editing ----------------

bool HotkeyChordStyle::RebindCommand(const std::string& commandName, const FHotkeyChord& newChord, int slotIndex)
{
    FHotkeyCommand* cmd = FindCommandMutable(commandName);
    if (!cmd || slotIndex < 0)
        return false;

    FHotkeyChord normalized = newChord;
    NormalizeChord(normalized);

    if (static_cast<size_t>(slotIndex) >= cmd->chords.size())
    {
        // If slot doesn't exist, grow up to slot and insert
        cmd->chords.resize(static_cast<size_t>(slotIndex) + 1);
    }

    cmd->chords[static_cast<size_t>(slotIndex)] = normalized;
    return true;
}

bool HotkeyChordStyle::AddAlternateChord(const std::string& commandName, const FHotkeyChord& chord)
{
    FHotkeyCommand* cmd = FindCommandMutable(commandName);
    if (!cmd)
        return false;

    FHotkeyChord normalized = chord;
    NormalizeChord(normalized);

    // Avoid duplicates
    for (const FHotkeyChord& c : cmd->chords)
    {
        if (ChordsEqual(c, normalized))
            return true;
    }

    cmd->chords.push_back(std::move(normalized));
    return true;
}

bool HotkeyChordStyle::RemoveChord(const std::string& commandName, int slotIndex)
{
    FHotkeyCommand* cmd = FindCommandMutable(commandName);
    if (!cmd || slotIndex < 0 || static_cast<size_t>(slotIndex) >= cmd->chords.size())
        return false;

    cmd->chords.erase(cmd->chords.begin() + slotIndex);
    return true;
}

bool HotkeyChordStyle::ResetCommandToDefault(const std::string& commandName)
{
    FHotkeyCommand* cmd = FindCommandMutable(commandName);
    if (!cmd)
        return false;

    cmd->chords = cmd->defaultChords;
    for (FHotkeyChord& c : cmd->chords)
        NormalizeChord(c);

    return true;
}

void HotkeyChordStyle::ResetAllToDefaults()
{
    for (FHotkeyCommand& cmd : m_Map.commands)
    {
        cmd.chords = cmd.defaultChords;
        for (FHotkeyChord& c : cmd.chords)
            NormalizeChord(c);
    }
}

// ---------------- Persistence ----------------

void HotkeyChordStyle::ApplyOverrides(const FHotkeyOverrides& overrides)
{
    for (const FHotkeyOverrideEntry& entry : overrides.entries)
    {
        FHotkeyCommand* cmd = FindCommandMutable(entry.commandName);
        if (!cmd)
            continue;

        cmd->chords = entry.customChords;
        for (FHotkeyChord& c : cmd->chords)
            NormalizeChord(c);
    }
}

FHotkeyOverrides HotkeyChordStyle::ExportOverrides() const
{
    FHotkeyOverrides out;

    for (const FHotkeyCommand& cmd : m_Map.commands)
    {
        bool same = (cmd.chords.size() == cmd.defaultChords.size());
        if (same)
        {
            for (size_t i = 0; i < cmd.chords.size(); ++i)
            {
                if (!ChordsEqual(cmd.chords[i], cmd.defaultChords[i]))
                {
                    same = false;
                    break;
                }
            }
        }

        if (same)
            continue;

        FHotkeyOverrideEntry e;
        e.commandName = cmd.name;
        e.customChords = cmd.chords;
        out.entries.push_back(std::move(e));
    }

    return out;
}

// ---------------- UI ----------------

std::string HotkeyChordStyle::GetCommandDisplayString(const std::string& commandName) const
{
    const FHotkeyCommand* cmd = FindCommand(commandName);
    if (!cmd)
        return {};

    return HotkeyTextFormatter::CommandToString(*cmd, m_Platform);
}

// ---------------- Conflicts ----------------

std::vector<FHotkeyConflict> HotkeyChordStyle::FindConflicts(
    const FHotkeyChord& chord,
    const std::string& ignoreCommand) const
{
    std::vector<FHotkeyConflict> out;

    FHotkeyChord normalized = chord;
    NormalizeChord(normalized);

    for (const FHotkeyCommand& cmd : m_Map.commands)
    {
        if (!ignoreCommand.empty() && cmd.name == ignoreCommand)
            continue;

        for (const FHotkeyChord& c : cmd.chords)
        {
            if (!ChordAppliesToCurrentPlatform(c))
                continue;

            if (ChordsEqual(c, normalized))
            {
                FHotkeyConflict conflict;
                conflict.existingCommand = cmd.name;
                conflict.chord = c;
                out.push_back(std::move(conflict));
            }
        }
    }

    return out;
}

const FHotkeyCommand* HotkeyChordStyle::FindCommandInfo(const std::string& commandName) const
{
    return FindCommand(commandName);
}
// ---------------- Helpers ----------------

void HotkeyChordStyle::NormalizeChord(FHotkeyChord& chord, bool bCanonicalize)
{
    if (bCanonicalize)
    {
        // 1) Canonicalize modifier sides (RightCtrl -> LeftCtrl, etc.)
        for (EPhysicalInput& k : chord.keys)
        {
            k = CanonicalizeModifier(k);
        }
    }

    // 2) Remove Unknown keys
    chord.keys.erase(
        std::remove(chord.keys.begin(), chord.keys.end(), EPhysicalInput::Unknown),
        chord.keys.end());

    // 3) Sort (modifiers first, then enum value)
    std::sort(chord.keys.begin(), chord.keys.end(),
        [](EPhysicalInput a, EPhysicalInput b)
        {
            const int pa = KeySortPriority(a);
            const int pb = KeySortPriority(b);
            if (pa != pb) return pa < pb;
            return static_cast<uint16_t>(a) < static_cast<uint16_t>(b);
        });

    // 4) Unique
    chord.keys.erase(std::unique(chord.keys.begin(), chord.keys.end()), chord.keys.end());
}

bool HotkeyChordStyle::ChordsEqual(const FHotkeyChord& a, const FHotkeyChord& b)
{
    return a.platforms == b.platforms &&
           a.allowExtraModifiers == b.allowExtraModifiers &&
           a.allowExtraKeys == b.allowExtraKeys &&
           a.keys == b.keys;
}

bool HotkeyChordStyle::ChordAppliesToCurrentPlatform(const FHotkeyChord& chord) const
{
    if (chord.platforms == EHotkeyPlatformMask::Any)
        return true;

    return HasAnyFlag(chord.platforms, ToPlatformMask(m_Platform));
}

const FInputDeviceState* HotkeyChordStyle::FindKeyboard(const std::vector<FInputDeviceState>& devices) const
{
    for (const FInputDeviceState& d : devices)
    {
        if (d.type == EInputDeviceType::Keyboard)
            return &d;
    }
    return nullptr;
}

bool HotkeyChordStyle::IsKeyDown(const FInputDeviceState* keyboard, EPhysicalInput key) const
{
    if (!keyboard)
        return false;

    auto it = keyboard->values.find(key);
    if (it == keyboard->values.end())
        return false;

    return it->second > 0.5f;
}

bool HotkeyChordStyle::AnyExtraKeyDown(
    const FHotkeyChord& chord,
    const std::vector<FInputDeviceState>& devices,
    bool modifiersOnly) const
{
    const FInputDeviceState* kb = FindKeyboard(devices);
    if (!kb)
        return false;

    for (const auto& kv : kb->values)
    {
        const EPhysicalInput key = kv.first;
        const float v = kv.second;

        if (v <= 0.5f)
            continue;

        if (!IsKeyboard(key))
            continue;

        if (ContainsKey(chord.keys, key))
            continue;

        if (modifiersOnly && !IsModifierKey(key))
            continue;

        return true;
    }

    return false;
}

bool HotkeyChordStyle::IsChordHeld(const FHotkeyChord& chord, const std::vector<FInputDeviceState>& devices) const
{
    if (chord.keys.empty())
        return false;

    const FInputDeviceState* kb = FindKeyboard(devices);
    if (!kb)
        return false;

    // All required keys must be down
    for (EPhysicalInput k : chord.keys)
    {
        if (!IsKeyDown(kb, k))
            return false;
    }

    // Strictness options
    if (!chord.allowExtraModifiers)
    {
        if (AnyExtraKeyDown(chord, devices, /*modifiersOnly*/ true))
            return false;
    }

    if (!chord.allowExtraKeys)
    {
        if (AnyExtraKeyDown(chord, devices, /*modifiersOnly*/ false))
            return false;
    }

    return true;
}

bool HotkeyChordStyle::HasNewPressInChord(
    const FHotkeyChord& chord,
    const std::vector<FInputDeviceState>& devices,
    const std::vector<FInputDeviceState>& prevDevices) const
{
    const FInputDeviceState* kbNow  = FindKeyboard(devices);
    const FInputDeviceState* kbPrev = FindKeyboard(prevDevices);

    // Require at least one chord key to transition up->down this frame
    for (EPhysicalInput k : chord.keys)
    {
        const bool nowDown  = IsKeyDown(kbNow, k);
        const bool prevDown = IsKeyDown(kbPrev, k);

        if (nowDown && !prevDown)
            return true;
    }

    return false;
}

FHotkeyCommand* HotkeyChordStyle::FindCommandMutable(const std::string& name)
{
    for (FHotkeyCommand& cmd : m_Map.commands)
    {
        if (cmd.name == name)
            return &cmd;
    }
    return nullptr;
}

const FHotkeyCommand* HotkeyChordStyle::FindCommand(const std::string& name) const
{
    for (const FHotkeyCommand& cmd : m_Map.commands)
    {
        if (cmd.name == name)
            return &cmd;
    }
    return nullptr;
}
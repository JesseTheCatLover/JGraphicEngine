//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Core/Reflection/REMeta.h"

REMetaSchema::REMetaSchema()
{
    // IMPORTANT: aliases live here so we can rename user-facing tokens later.
    // We can add more aliases without changing serialized data.
    m_Specs = {
        // ---- Editor exposure ----
        {REMetaID::HiddenInInspector, {"HiddenInInspector", "HiddenToInspector", "NoInspector", "Invisible"}, false},
        {REMetaID::EditAnywhere, {"EditAnywhere", "Edit"}, false},
        {REMetaID::EditSchemaOnly, {"EditSchemaOnly"}, false},
        {REMetaID::EditInstanceOnly, {"EditInstanceOnly"}, false},

        {REMetaID::VisibleAnywhere, {"VisibleAnywhere", "Visible"}, false},
        {REMetaID::VisibleSchemaOnly, {"VisibleSchemaOnly"}, false},
        {REMetaID::VisibleInstanceOnly, {"VisibleInstanceOnly"}, false},

        // ---- Script exposure ----
        {REMetaID::Scriptable, {"Scriptable"}, false},
        {REMetaID::ScriptReadonly, {"ScriptReadonly", "ScriptReadOnly"}, false},
        {REMetaID::HiddenToScript, {"HiddenToScript"}, false},

        // ---- Serialization flags ----
        {REMetaID::Transient, {"Transient"}, false},
        {REMetaID::SaveGame, {"SaveGame"}, false},
        {REMetaID::SkipSerialization, {"SkipSerialization", "SkipSerialize"}, false},

        // ---- Presentation ----
        {REMetaID::Category, {"Category"}, true},
        {REMetaID::DisplayName, {"Display", "DisplayName"}, true},
        {REMetaID::Tooltip, {"Tooltip", "ToolTip"}, true},
        {REMetaID::AdvancedDisplay, {"AdvancedDisplay", "Advanced"}, false},
        {REMetaID::DisableCondition, {"DisableCondition"}, true},
        {REMetaID::HideCondition, {"HideCondition"}, true},
        {REMetaID::NoResetToDefault, {"NoResetToDefault"}, false},

        // ---- Numeric ----
        {REMetaID::Clamp, {"Clamp"}, true},
        {REMetaID::ClampMin, {"ClampMin"}, true},
        {REMetaID::ClampMax, {"ClampMax"}, true},

        {REMetaID::Range, {"Range"}, true},
        {REMetaID::RangeMin, {"RangeMin"}, true},
        {REMetaID::RangeMax, {"RangeMax"}, true},

        {REMetaID::Step, {"Step"}, true},
        {REMetaID::Units, {"Units"}, true},
        {REMetaID::Multiple, {"Multiple"}, true},
        {REMetaID::Precision, {"Precision"}, true},
    };
}
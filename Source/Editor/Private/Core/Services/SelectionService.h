//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <vector>

#include "Core/IEditorService.h"

struct FSelectionModifiers;
class EditorRuntime;

class SelectionService : public IEditorService
{
private:
    using ActorID = uint64_t;

    EditorRuntime& m_Runtime;
    std::vector<ActorID> m_Selected;
    ActorID m_Anchor = 0;
    ActorID m_RevealRequest = 0;

    void PushToRuntime();

    void SelectSingle(ActorID id);

    void Toggle(ActorID id);

    void SelectRangeTo(ActorID id, const std::vector<ActorID>& order);

public:
    explicit SelectionService(EditorRuntime& rt);

    [[nodiscard]] const std::vector<ActorID>& GetSelection() const { return m_Selected; }
    [[nodiscard]] ActorID GetAnchor() const { return m_Anchor; }

    [[nodiscard]] bool IsSelected(ActorID id) const;

    [[nodiscard]] bool IsSelectionEmpty() const;

    void ApplyClick(ActorID id, const FSelectionModifiers& mods,
                    const std::vector<ActorID>* visibleOrder /*nullable*/);

    ActorID ConsumeRevealRequest();

    void Clear();
};

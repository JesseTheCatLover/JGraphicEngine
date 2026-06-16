//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cstdint>
#include <string>

#include "EditorCore/IEditorService.h"
#include "TSelectionModel.h"

class SelectionService : public IEditorService
{
public:
    using ActorID = uint64_t;

private:
    TSelectionModel<ActorID> m_SceneActorSelection;
    TSelectionModel<std::string> m_AssetPathSelection;

public:
    explicit SelectionService() = default;

    TSelectionModel<ActorID>& GetSceneActorSelection() { return m_SceneActorSelection; }
    [[nodiscard]] const TSelectionModel<ActorID>& GetSceneActorSelection() const { return m_SceneActorSelection; }

    TSelectionModel<std::string>& GetAssetPathSelection() { return m_AssetPathSelection; }
    [[nodiscard]] const TSelectionModel<std::string>& GetAssetPathSelection() const { return m_AssetPathSelection; }
};

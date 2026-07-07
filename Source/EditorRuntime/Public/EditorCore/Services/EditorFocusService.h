//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

#include "EditorCore/IEditorService.h"

class EditorFocusService : public IEditorService
{
private:
    std::string m_ActiveContext = "Global";

public:
    // The UI calls this when a panel gains focus
    void SetActiveContext(const std::string& contextID)
    {
        m_ActiveContext = contextID;
    }

    [[nodiscard]] const std::string& GetActiveContext() const
    {
        return m_ActiveContext;
    }
};

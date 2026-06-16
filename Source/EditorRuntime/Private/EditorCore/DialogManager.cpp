//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.
// DialogManager.cpp

#include "EditorCore/DialogManager.h"

void DialogManager::ApplyPendingRequests()
{
    if (m_PendingDialogs.empty())
        return;

    for (auto& dlg : m_PendingDialogs)
        m_Dialogs.emplace_back(std::move(dlg));

    m_PendingDialogs.clear();
}

void DialogManager::DrawDialogs()
{
    // 1) Bring in any dialogs requested before this frame
    ApplyPendingRequests();

    // 2) Draw only dialogs that existed at frame start.
    // If a dialog requests opening another dialog during Draw(),
    // it goes into m_PendingDialogs and will appear next frame.
    const std::size_t count = m_Dialogs.size();

    for (std::size_t i = 0; i < count; ++i)
    {
        IEditorDialog* dlg = m_Dialogs[i].get();
        dlg->Draw(m_Host, m_Runtime);
    }

    // 3) Destroy closed dialogs after draw
    for (auto it = m_Dialogs.begin(); it != m_Dialogs.end();)
    {
        IEditorDialog* dlg = it->get();
        if (!dlg->IsOpen())
        {
            dlg->OnDestroy(m_Host, m_Runtime);
            it = m_Dialogs.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
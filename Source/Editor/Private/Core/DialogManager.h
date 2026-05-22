//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <vector>
#include "Core/Memory/SmartPointers.h"
#include "UI/IEditorDialog.h"

class EditorHost;
class EditorRuntime;

/**
 * @class DialogManager
 * @brief Owns, draws, and controls the lifetime of editor dialogs.
 *
 * ## Threading:
 * This manager is intended to be used from the UI thread only.
 */
class DialogManager
{
private:
    friend class IEditorRenderer;
    friend class ImGuiRenderer;

    EditorHost& m_Host;
    EditorRuntime& m_Runtime;
    std::vector<TUniquePtr<IEditorDialog>> m_Dialogs; ///< Active dialogs (drawn every frame)
    std::vector<TUniquePtr<IEditorDialog>> m_PendingDialogs; ///< Dialogs requested during a frame; appended at a safe point

    // Apply all queued open requests
    void ApplyPendingRequests();

    // Draws all dialogs; called by renderer
    void DrawDialogs();

public:
    DialogManager(EditorHost& host, EditorRuntime& runtime)
        : m_Host(host)
        , m_Runtime(runtime)
    {}

    template<typename TDialog>
    TDialog* FindDialogInstance()
    {
        for (auto& dlg : m_Dialogs)
            if (auto* casted = dynamic_cast<TDialog*>(dlg.get()))
                return casted;
        return nullptr;
    }

    /**
     * @brief Safely opens (or focuses) a dialog.
     *
     * This is the **preferred** way to open dialogs and is safe to call from:
     * - panels/tools
     * - other dialogs (`IEditorDialog::Draw()`)
     *
     * @tparam TDialog Dialog type deriving from `IEditorDialog`.
     * @return Pointer to the (existing or newly created) dialog instance.
     *
     * @warning Newly created dialogs are not added to the active draw list immediately; they become
     *          active when pending requests are applied (typically the next frame / safe point).
     */
    template<typename TDialog>
    TDialog* RequestOpenDialog()
    {
        // Check already active
        if (TDialog* existing = FindDialogInstance<TDialog>())
        {
            if (!existing->IsOpen())
                existing->OnOpen(m_Host, m_Runtime);
            else
                existing->OnRequestFocus(m_Host, m_Runtime);

            return existing;
        }

        // Create new dialog into pending list
        auto ptr = MakeUnique<TDialog>();
        TDialog* instance = ptr.get();

        instance->OnCreate(m_Host, m_Runtime);
        instance->OnOpen(m_Host, m_Runtime);

        m_PendingDialogs.emplace_back(std::move(ptr));
        return instance;
    }

    /**
     * @brief Opens (or focuses) a dialog immediately by mutating the active dialog list.
     *
     * This API exists for legacy/internal uses but is **not safe** to call from within dialog drawing.
     *
     * @tparam TDialog Dialog type deriving from `IEditorDialog`.
     * @return Pointer to the (existing or newly created) dialog instance.
     *
     * @warning **Do not call this from `IEditorDialog::Draw()` or while dialogs are being iterated.**
     *          Prefer @ref RequestOpenDialog().
     */
    template<typename TDialog>
    TDialog* OpenDialog()
    {
        TDialog* instance = FindDialogInstance<TDialog>();
        if (!instance)
        {
            auto ptr = MakeUnique<TDialog>();
            instance = ptr.get();
            instance->OnCreate(m_Host, m_Runtime);
            m_Dialogs.emplace_back(std::move(ptr));

            // First open
            instance->OnOpen(m_Host, m_Runtime);
            return instance;
        }

        // Is Instance already open?
        const bool bIsOpen = instance->IsOpen();

        if (!bIsOpen)
        {
            // Re-opening a closed dialog
            instance->OnOpen(m_Host, m_Runtime);
        }
        else
        {
            // Dialog is already open: just focus it
            instance->OnRequestFocus(m_Host, m_Runtime);
        }

        return instance;
    }
    /**
   * @brief Closes a dialog (if it exists).
   *
   * Behavior:
   * - If an instance exists, calls `OnClose(...)`.
   * - Actual destruction/removal is performed later by the manager at a safe point.
   *
   * @tparam TDialog Dialog type deriving from `IEditorDialog`.
   */
    template<typename TDialog>
    void CloseDialog()
    {
        if (TDialog* dlg = FindDialogInstance<TDialog>())
            dlg->OnClose(m_Host, m_Runtime);
    }
};
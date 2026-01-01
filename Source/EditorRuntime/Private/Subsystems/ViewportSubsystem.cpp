//  Copyright 2026 JesseTheCatLover. All Rights Reserved.

#include "Subsystems/ViewportSubsystem.h"

#include "EditorRuntime.h"

ViewportSubsystem::ViewportSubsystem(EditorHost &host, EditorRuntime &runtime, ToolService &tools)
: m_Host(host)
          , m_Runtime(runtime)
          , m_Tools(tools)
          , m_Channel([this](PanelID id)
              {
                  return MakeUnique<ViewportController>(id, m_Host, m_Runtime, m_Tools);
              })
{}

bool ViewportSubsystem::IsCaptureOwner(PanelID id) const
{
    return (id != 0) && (id == m_CaptureOwner) && (m_SharedCaptureKind != EMouseCaptureKind::None);
}

bool ViewportSubsystem::TryBeginCapture(PanelID id, EMouseCaptureKind kind)
{
    if (id == 0 || kind == EMouseCaptureKind::None)
        return false;

    // Already owned by same panel => allow upgrading/changing kind.
    if (m_CaptureOwner == id)
    {
        m_SharedCaptureKind = kind;
        ApplySurfaceCursorCapture(true);
        return true;
    }

    // If someone else owns it, reject.
    if (m_CaptureOwner != 0)
        return false;

    // Acquire.
    m_CaptureOwner = id;
    m_SharedCaptureKind  = kind;
    ApplySurfaceCursorCapture(true);
    return true;
}

void ViewportSubsystem::EndCapture(PanelID id)
{
    if (id == 0)
        return;

    if (m_CaptureOwner != id)
        return; // only owner can release

    m_CaptureOwner = 0;
    m_SharedCaptureKind  = EMouseCaptureKind::None;
    ApplySurfaceCursorCapture(false);
}
void ViewportSubsystem::ApplySurfaceCursorCapture(const bool bShouldCapture)
{
    if (bShouldCapture && !m_bCursorCaptured)
    {
        m_Runtime.GetSurface().SetCursorDisabled();
        m_bCursorCaptured = true;
    }
    else if (m_bCursorCaptured)
    {
        m_Runtime.GetSurface().SetCursorVisible();
        m_bCursorCaptured = false;
    }
}
//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

class DockSpace
{
public:
    // Call at the beginning of the UI frame before drawing any panels.
    void Begin();

    // Call at the end of the UI frame after drawing all panels.
    void End();
};
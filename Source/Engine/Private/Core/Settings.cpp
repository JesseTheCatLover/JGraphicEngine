// Copyright (c) 2024 JesseTheCatLover. All Rights Reserved.

#include "../../Public/Core/Settings.h"

Settings::Settings(): Window{1920, 1080}, Viewport{false}
{}

Settings::Settings(unsigned int Width, unsigned int Height, bool bWireframe):
Window{Width, Height}, Viewport{bWireframe}
{}

Settings::Settings(S_Window Win, S_Viewport View):
Window(Win), Viewport(View)
{}

Settings::Settings(S_Window Win, S_Viewport View, S_Camera Camera):
        Window(Win), Viewport(View), Camera(Camera)
{}

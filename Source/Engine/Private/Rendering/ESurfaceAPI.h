//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string_view>

enum class ESurfaceAPI { GLFW, Null};
inline ESurfaceAPI ParseSurfaceAPI(std::string_view str)
{
    if (str == "GLFW") return ESurfaceAPI::GLFW;
    if (str == "Null") return ESurfaceAPI::Null;
    return ESurfaceAPI::GLFW; // Default fallback
}
//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string_view>

enum class EGraphicsAPI { OpenGL, Vulkan, Metal, DirectX, Null};
inline EGraphicsAPI ParseGraphicsAPI(std::string_view str)
{
    if (str == "OpenGL") return EGraphicsAPI::OpenGL;
    if (str == "Vulkan") return EGraphicsAPI::Vulkan;
    if (str == "Metal") return EGraphicsAPI::Metal;
    if (str == "DirectX") return EGraphicsAPI::DirectX;
    if (str == "Null") return EGraphicsAPI::Null;
    return EGraphicsAPI::OpenGL; // Default fallback
}
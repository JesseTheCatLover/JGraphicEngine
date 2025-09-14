// Copyright (c) 2025 JesseTheCatLover. All Rights Reserved.

#include "JTexture.h"
#include <iostream>
#include <stb_image.h>

// Standard cubemap face names
static const std::vector<std::string> kCubemapNames = {
    "right", "left", "top", "bottom", "front", "back"
};

// Supported extensions
static const std::vector<std::string> kCubemapExtensions = {
    ".jpg", ".png", ".tga", ".bmp"
};

JTexture::JTexture(const std::string& path, TextureType type)
    : type(type)
{
    if (type == TextureType::Texture2D) {
        Load2D(path);
    } else if (type == TextureType::CubeMap) {
        LoadCubeMap(path);
    }
}

void JTexture::Bind(GLenum unit) const
{
    glActiveTexture(unit);
    if (type == TextureType::Texture2D) {
        glBindTexture(GL_TEXTURE_2D, texture);
    } else if (type == TextureType::CubeMap) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    }
}

void JTexture::Load2D(const std::string& fileName)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Wrapping & filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::string fullPath = std::string(ENGINE_DIRECTORY) + "/Resources/Textures/" + fileName;
    data = stbi_load(fullPath.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        GenerateTexture(nrChannels <= 3 ? GL_RGB : GL_RGBA);
    } else {
        std::cerr << "ERROR::TEXTURE::FILE_NOT_SUCCESSFULLY_READ: " << fileName << std::endl;
    }

    stbi_image_free(data);
    data = nullptr;
}

void JTexture::LoadCubeMap(const std::string& directory)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    for (unsigned int i = 0; i < kCubemapNames.size(); i++)
        {
        std::string filePath;
        unsigned char* faceData = nullptr;

        // Try all supported extensions
        for (const auto& ext : kCubemapExtensions)
        {
            filePath = std::string(ENGINE_DIRECTORY) + "/Resources/Skyboxes/" +
                       directory + "/" + kCubemapNames[i] + ext;

            faceData = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);
            if (faceData) break;
        }

        if (faceData)
        {
            GLint format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format,
                         width, height, 0, format, GL_UNSIGNED_BYTE, faceData);
            stbi_image_free(faceData);
        } else {
            std::cerr << "ERROR::CUBEMAP::FAILED_TO_LOAD_FACE: "
                      << kCubemapNames[i] << " in " << directory << std::endl;
        }
    }

    // Cubemap settings
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void JTexture::GenerateTexture(const GLint &colorFormat) {
    glTexImage2D(GL_TEXTURE_2D, 0, colorFormat, width, height, 0, colorFormat,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
}

JTexture::~JTexture()
{
    glDeleteTextures(1, &texture);
}


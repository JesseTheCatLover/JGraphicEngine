// Copyright (c) 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <glad/gl.h>
#include <string>

enum class TextureType
{
    Texture2D,
    CubeMap
};

class JTexture
{
public:
    explicit JTexture(const std::string& path, TextureType type = TextureType::Texture2D);
    void Bind(GLenum unit) const;
    ~JTexture();
private:
    GLuint texture;
    int width, height, nrChannels;
    unsigned char* data = nullptr;

    TextureType type;

    void Load2D(const std::string& fileName);
    void LoadCubeMap(const std::string& directory);
    void GenerateTexture(const GLint &colorFormat);
};

// Copyright (c) 2024. JesseTheCatLover. All Rights Reserved.

#pragma once
#include "glad/glad.h"
#include <string>

class JTexture
{
public:
    JTexture(const std::string &fileName);
    void Bind();
    ~JTexture();
private:
    GLuint texture;
    int width, height, nrChannels;
    unsigned char* data;
    void GenerateTexture(const GLint &colorFormat);
};

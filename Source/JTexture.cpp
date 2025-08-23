// Copyright (c) 2024. JesseTheCatLover. All Rights Reserved.

#include <iostream>
#include "JTexture.h"
#include "ThirdParty/stb_image.h"

JTexture::JTexture(const std::string &fileName)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    data = stbi_load((std::string(ENGINE_DIRECTORY) + "/Resources/Textures/" + fileName).c_str(), &width, &height, &nrChannels, 0);

    if(data)
    {
        GenerateTexture(nrChannels <= 3 ? GL_RGB : GL_RGBA);
    }
    else
    {
        std::cerr << "ERROR::TEXTURE::FILE_NOT_SUCCESSFULLY_READ: " << fileName << std::endl;
    }

    stbi_image_free(data);
}

void JTexture::Bind()
{
    glBindTexture(GL_TEXTURE_2D, texture);
}

void JTexture::GenerateTexture(const GLint &colorFormat)
{
    glTexImage2D(GL_TEXTURE_2D, 0, colorFormat, width, height, 0, colorFormat,
            GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
}

JTexture::~JTexture()
{
    glDeleteTextures(1, &texture);
}


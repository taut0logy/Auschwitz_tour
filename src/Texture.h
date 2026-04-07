#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef GL_TEXTURE_MAX_ANISOTROPY
#define GL_TEXTURE_MAX_ANISOTROPY 0x84FE
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY
#define GL_MAX_TEXTURE_MAX_ANISOTROPY 0x84FF
#endif

inline unsigned int loadTexture(const char* path,
                                GLenum wrapMode = GL_REPEAT,
                                bool useMipmap = true,
                                bool useAnisotropic = false)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    // Check if file exists first
    std::cerr << "  [tex] " << path << " ... " << std::flush;
    {
        std::ifstream test(path);
        if (!test.good()) {
            std::cerr << "NOT FOUND" << std::endl;
            glDeleteTextures(1, &textureID);
            return 0;
        }
    }

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 1)      format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        if (useMipmap) {
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

        if (useAnisotropic) {
            float maxAniso = 1.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
        }

        std::cerr << "Texture loaded: " << path << " (" << width << "x" << height
                  << ", " << nrChannels << " ch)" << std::endl;
        stbi_image_free(data);
        return textureID;
    }
    else {
        std::cerr << "Texture skipped (not found): " << path << std::endl;
        glDeleteTextures(1, &textureID);
        return 0;
    }
}


// Create a texture from raw RGBA data (for procedural generation)
inline unsigned int createTextureFromData(unsigned char* data, int width, int height,
                                          GLenum wrapMode = GL_CLAMP_TO_EDGE)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    return textureID;
}

#endif

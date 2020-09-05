#pragma once

#include <Common.h>

#include <GL/glew.h>


// XXX: Remove when we switch to Vulkan.
GLuint loadShaderProgram(
    const std::string& blitVertexShaderCode,
    const std::string& blitFragmentShaderCode);


class Blitter {

public:

    Blitter();

    ~Blitter();

    void importSemaphores(int waitSemaphoreFd, int signalSemaphoreFd);

    void deleteTexture();

    void createTexture(
        int width,
        int height,
        int colorFd,
        uint64_t colorMemorySize,
        int depthFd,
        uint64_t depthMemorySize);

    void blit();

private:

    GLuint _blitProgramId;
    GLuint _waitSemaphoreId = 0;
    GLuint _signalSemaphoreId = 0;

    GLuint _colorMemoryObjectId = 0;
    GLuint _colorTextureId = 0;
    GLuint _depthMemoryObjectId = 0;
    GLuint _depthTextureId = 0;

};

#pragma once

#include <Common.h>

#include <GL/glew.h>

#define GL_HANDLE_TYPE_OPAQUE_FD_EXT 0x9586
#define GL_LAYOUT_COLOR_ATTACHMENT_EXT 0x958E
#define GL_LAYOUT_DEPTH_STENCIL_ATTACHMENT_EXT 0x958F

void HVRT_glCreateMemoryObjectsEXT(GLsizei n, GLuint* memoryObjects);

void HVRT_glDeleteMemoryObjectsEXT(GLsizei n, GLuint* memoryObjects);

void HVRT_glImportMemoryFdEXT(GLuint memory, GLuint64 size, GLenum handleType, GLint fd);

void HVRT_glTexStorageMem2DEXT(
    GLenum target,
    GLsizei levels,
    GLenum internalFormat,
    GLsizei width,
    GLsizei height,
    GLuint memory,
    GLuint64 offset);

void HVRT_glImportSemaphoreFdEXT(GLuint semaphore, GLenum handleType, GLint fd);

void HVRT_glGenSemaphoresEXT(GLsizei n, GLuint* semaphores);

void HVRT_glDeleteSemaphoresEXT(GLsizei n, const GLuint* semaphores);

void HVRT_glWaitSemaphoreEXT(
    GLuint semaphore,
    GLuint numBufferBarriers,
    const GLuint* buffers,
    GLuint numTextureBarriers,
    const GLuint* textures,
    const GLenum* srcLayouts);

void HVRT_glSignalSemaphoreEXT(
    GLuint semaphore,
    GLuint numBufferBarriers,
    const GLuint* buffers,
    GLuint numTextureBarriers,
    const GLuint* textures,
    const GLenum* dstLayouts);

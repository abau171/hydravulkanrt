#include <Common.h>

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>


static PFNGLCREATEMEMORYOBJECTSEXTPROC glCreateMemoryObjectsEXT_ptr = nullptr;
void HVRT_glCreateMemoryObjectsEXT(GLsizei n, GLuint* memoryObjects) {
    if (glCreateMemoryObjectsEXT_ptr == nullptr) {
        glCreateMemoryObjectsEXT_ptr = reinterpret_cast<PFNGLCREATEMEMORYOBJECTSEXTPROC>(
            glXGetProcAddress(reinterpret_cast<const GLubyte*>("glCreateMemoryObjectsEXT")));
    }
    (*glCreateMemoryObjectsEXT_ptr)(n, memoryObjects);
}

static PFNGLDELETEMEMORYOBJECTSEXTPROC glDeleteMemoryObjectsEXT_ptr = nullptr;
void HVRT_glDeleteMemoryObjectsEXT(GLsizei n, GLuint* memoryObjects) {
    if (glDeleteMemoryObjectsEXT_ptr == nullptr) {
        glDeleteMemoryObjectsEXT_ptr = reinterpret_cast<PFNGLDELETEMEMORYOBJECTSEXTPROC>(
            glXGetProcAddress(reinterpret_cast<const GLubyte*>("glDeleteMemoryObjectsEXT")));
    }
    (*glDeleteMemoryObjectsEXT_ptr)(n, memoryObjects);
}

static PFNGLIMPORTMEMORYFDEXTPROC glImportMemoryFdEXT_ptr = nullptr;
void HVRT_glImportMemoryFdEXT(GLuint memory, GLuint64 size, GLenum handleType, GLint fd) {
    if (glImportMemoryFdEXT_ptr == nullptr) {
        glImportMemoryFdEXT_ptr = reinterpret_cast<PFNGLIMPORTMEMORYFDEXTPROC>(
            glXGetProcAddress(reinterpret_cast<const GLubyte*>("glImportMemoryFdEXT")));
    }
    (*glImportMemoryFdEXT_ptr)(memory, size, handleType, fd);
}

static PFNGLTEXSTORAGEMEM2DEXTPROC glTexStorageMem2DEXT_ptr = nullptr;
void HVRT_glTexStorageMem2DEXT(
    GLenum target,
    GLsizei levels,
    GLenum internalFormat,
    GLsizei width,
    GLsizei height,
    GLuint memory,
    GLuint64 offset)
{
    if (glTexStorageMem2DEXT_ptr == nullptr) {
        glTexStorageMem2DEXT_ptr = reinterpret_cast<PFNGLTEXSTORAGEMEM2DEXTPROC>(
            glXGetProcAddress(reinterpret_cast<const GLubyte*>("glTexStorageMem2DEXT")));
    }
    (*glTexStorageMem2DEXT_ptr)(target, levels, internalFormat, width, height, memory, offset);
}

static PFNGLIMPORTSEMAPHOREFDEXTPROC glImportSemaphoreFdEXT_ptr = nullptr;
void HVRT_glImportSemaphoreFdEXT(GLuint semaphore, GLenum handleType, GLint fd) {
    if (glImportSemaphoreFdEXT_ptr == nullptr) {
        glImportSemaphoreFdEXT_ptr = reinterpret_cast<PFNGLIMPORTSEMAPHOREFDEXTPROC>(
            glXGetProcAddress(reinterpret_cast<const GLubyte*>("glImportSemaphoreFdEXT")));
    }
    (*glImportSemaphoreFdEXT_ptr)(semaphore, handleType, fd);
}

static PFNGLGENSEMAPHORESEXTPROC glGenSemaphoresEXT_ptr = nullptr;
void HVRT_glGenSemaphoresEXT(GLsizei n, GLuint* semaphores) {
    if (glGenSemaphoresEXT_ptr == nullptr) {
        glGenSemaphoresEXT_ptr = reinterpret_cast<PFNGLGENSEMAPHORESEXTPROC>(
            glXGetProcAddress(reinterpret_cast<const GLubyte*>("glGenSemaphoresEXT")));
    }
    (*glGenSemaphoresEXT_ptr)(n, semaphores);
}

static PFNGLDELETESEMAPHORESEXTPROC glDeleteSemaphoresEXT_ptr = nullptr;
void HVRT_glDeleteSemaphoresEXT(GLsizei n, const GLuint* semaphores) {
    if (glDeleteSemaphoresEXT_ptr == nullptr) {
        glDeleteSemaphoresEXT_ptr = reinterpret_cast<PFNGLDELETESEMAPHORESEXTPROC>(
            glXGetProcAddress(reinterpret_cast<const GLubyte*>("glDeleteSemaphoresEXT")));
    }
    (*glDeleteSemaphoresEXT_ptr)(n, semaphores);
}

static PFNGLWAITSEMAPHOREEXTPROC glWaitSemaphoreEXT_ptr = nullptr;
void HVRT_glWaitSemaphoreEXT(
    GLuint semaphore,
    GLuint numBufferBarriers,
    const GLuint* buffers,
    GLuint numTextureBarriers,
    const GLuint* textures,
    const GLenum* srcLayouts)
{
    if (glWaitSemaphoreEXT_ptr == nullptr) {
        glWaitSemaphoreEXT_ptr = reinterpret_cast<PFNGLWAITSEMAPHOREEXTPROC>(
            glXGetProcAddress(reinterpret_cast<const GLubyte*>("glWaitSemaphoreEXT")));
    }
    (*glWaitSemaphoreEXT_ptr)(
        semaphore,
        numBufferBarriers,
        buffers,
        numTextureBarriers,
        textures,
        srcLayouts);
}

static PFNGLSIGNALSEMAPHOREEXTPROC glSignalSemaphoreEXT_ptr = nullptr;
void HVRT_glSignalSemaphoreEXT(
    GLuint semaphore,
    GLuint numBufferBarriers,
    const GLuint* buffers,
    GLuint numTextureBarriers,
    const GLuint* textures,
    const GLenum* dstLayouts)
{
    if (glSignalSemaphoreEXT_ptr == nullptr) {
        glSignalSemaphoreEXT_ptr = reinterpret_cast<PFNGLSIGNALSEMAPHOREEXTPROC>(
            glXGetProcAddress(reinterpret_cast<const GLubyte*>("glSignalSemaphoreEXT")));
    }
    (*glSignalSemaphoreEXT_ptr)(
        semaphore,
        numBufferBarriers,
        buffers,
        numTextureBarriers,
        textures,
        dstLayouts);
}

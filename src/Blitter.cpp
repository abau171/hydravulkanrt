#include <Common.h>

#include <GL/glew.h>

#include <HVRTGL.h>
#include <Blitter.h>


const std::string blitVertexShaderCode = R"(
#version 450

layout(location = 0) in vec2 inPosition;

layout(location = 0) out vec2 fragUv;

void main() {
    gl_Position = vec4(inPosition, 0.999f, 1.0f);
    fragUv = 0.5f * inPosition.xy + vec2(0.5f);
};
)";

const std::string blitFragmentShaderCode = R"(
#version 450

layout(binding = 0) uniform sampler2D colorTexture;
layout(binding = 1) uniform sampler2D depthTexture;

layout(location = 0) in vec2 fragUv;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(texture(colorTexture, fragUv).rgb, 1.0f);
    gl_FragDepth = texture(depthTexture, fragUv).r;
};
)";


static GLuint loadShader(const std::string& code, GLenum type) {

    GLuint shaderId = glCreateShader(type);
    const GLchar* shaderCodePointer = code.c_str();
    glShaderSource(shaderId, 1, &shaderCodePointer, nullptr);

    glCompileShader(shaderId);

    GLint status;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {

        GLint logSize;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logSize);
        std::vector<GLchar> shaderCompileLog(logSize);
        glGetShaderInfoLog(shaderId, logSize, nullptr, shaderCompileLog.data());

        throw std::runtime_error(
            std::string()
            + "Blit shader compilation failed:\n"
            + shaderCompileLog.data());
    }

    return shaderId;
}

GLuint loadShaderProgram(
    const std::string& blitVertexShaderCode,
    const std::string& blitFragmentShaderCode)
{
    GLuint vertexShaderId = loadShader(blitVertexShaderCode, GL_VERTEX_SHADER);
    GLuint fragmentShaderId = loadShader(blitFragmentShaderCode, GL_FRAGMENT_SHADER);

    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);
    glLinkProgram(programId);

    glDetachShader(programId, vertexShaderId);
    glDeleteShader(vertexShaderId);
    glDetachShader(programId, fragmentShaderId);
    glDeleteShader(fragmentShaderId);

    GLint status;
    glGetProgramiv(programId, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {

        GLint logSize;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logSize);
        std::vector<GLchar> shaderLinkLog(logSize);
        glGetProgramInfoLog(programId, logSize, nullptr, shaderLinkLog.data());

        throw std::runtime_error(
            std::string()
            + "Blit shader linking failed:\n"
            + shaderLinkLog.data());
    }

    return programId;
}

Blitter::Blitter() {
    _blitProgramId = loadShaderProgram(blitVertexShaderCode, blitFragmentShaderCode);
}

Blitter::~Blitter() {
    glDeleteProgram(_blitProgramId);
    if (_waitSemaphoreId != 0) {
        HVRT_glDeleteSemaphoresEXT(1, &_waitSemaphoreId);
        _waitSemaphoreId = 0;
    }
    if (_signalSemaphoreId != 0) {
        HVRT_glDeleteSemaphoresEXT(1, &_signalSemaphoreId);
        _signalSemaphoreId = 0;
    }
    deleteTexture();
}

void Blitter::importSemaphores(int waitSemaphoreFd, int signalSemaphoreFd) {

    HVRT_glGenSemaphoresEXT(1, &_waitSemaphoreId);
    HVRT_glImportSemaphoreFdEXT(
        _waitSemaphoreId,
        GL_HANDLE_TYPE_OPAQUE_FD_EXT,
        waitSemaphoreFd);

    HVRT_glGenSemaphoresEXT(1, &_signalSemaphoreId);
    HVRT_glImportSemaphoreFdEXT(
        _signalSemaphoreId,
        GL_HANDLE_TYPE_OPAQUE_FD_EXT,
        signalSemaphoreFd);
}

void Blitter::deleteTexture() {
    if (_colorMemoryObjectId != 0) {
        HVRT_glDeleteMemoryObjectsEXT(1, &_colorMemoryObjectId);
        _colorMemoryObjectId = 0;
    }
    if (_colorTextureId != 0) {
        glDeleteTextures(1, &_colorTextureId);
        _colorTextureId = 0;
    }
    if (_depthMemoryObjectId != 0) {
        HVRT_glDeleteMemoryObjectsEXT(1, &_depthMemoryObjectId);
        _depthMemoryObjectId = 0;
    }
    if (_depthTextureId != 0) {
        glDeleteTextures(1, &_depthTextureId);
        _depthTextureId = 0;
    }
}

void Blitter::createTexture(
    int width,
    int height,
    int colorFd,
    uint64_t colorMemorySize,
    int depthFd,
    uint64_t depthMemorySize)
{
    HVRT_glCreateMemoryObjectsEXT(1, &_colorMemoryObjectId);
    HVRT_glImportMemoryFdEXT(
        _colorMemoryObjectId,
        colorMemorySize,
        GL_HANDLE_TYPE_OPAQUE_FD_EXT,
        colorFd);
    glGenTextures(1, &_colorTextureId);
    glBindTexture(GL_TEXTURE_2D, _colorTextureId);
    HVRT_glTexStorageMem2DEXT(GL_TEXTURE_2D, 1, GL_RGBA32F, width, height, _colorMemoryObjectId, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    HVRT_glCreateMemoryObjectsEXT(1, &_depthMemoryObjectId);
    HVRT_glImportMemoryFdEXT(
        _depthMemoryObjectId,
        depthMemorySize,
        GL_HANDLE_TYPE_OPAQUE_FD_EXT,
        depthFd);
    glGenTextures(1, &_depthTextureId);
    glBindTexture(GL_TEXTURE_2D, _depthTextureId);
    HVRT_glTexStorageMem2DEXT(GL_TEXTURE_2D, 1, GL_R32F, width, height, _depthMemoryObjectId, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Blitter::blit() {

    GLuint textures[] = { _colorTextureId, _depthTextureId };

    GLenum textureSrcLayouts[] = {
        GL_LAYOUT_COLOR_ATTACHMENT_EXT,
        GL_LAYOUT_DEPTH_STENCIL_ATTACHMENT_EXT
    };
    HVRT_glWaitSemaphoreEXT(
        _waitSemaphoreId,
        0,
        nullptr,
        0,
        textures,
        textureSrcLayouts);

    GLint oldDepthFunc;
    glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);
    glDepthFunc(GL_ALWAYS);

    GLboolean oldSRGBEnabled = glIsEnabled(GL_FRAMEBUFFER_SRGB);
    if (!oldSRGBEnabled) glEnable(GL_FRAMEBUFFER_SRGB);

    glUseProgram(_blitProgramId);

    glBindTextures(0, 2, textures);

    glBegin(GL_TRIANGLES);
    glVertex2f(-1.0f, 3.0f);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(3.0f, -1.0f);
    glEnd();

    glUseProgram(0);

    glDepthFunc(oldDepthFunc);
    if (!oldSRGBEnabled) glDisable(GL_FRAMEBUFFER_SRGB);

    GLenum textureDstLayouts[] = {
        GL_NONE,
        GL_NONE
    };
    HVRT_glSignalSemaphoreEXT(
        _signalSemaphoreId,
        0,
        nullptr,
        0,
        textures,
        textureDstLayouts);
}

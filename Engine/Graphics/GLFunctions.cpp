#include "GLFunctions.h"

#include <GLFW/glfw3.h>
#include <cstdio>

PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC glBufferData = nullptr;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;

PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;

PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;

PFNGLCREATESHADERPROC glCreateShader = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC glDeleteShader = nullptr;

PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
PFNGLATTACHSHADERPROC glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;

PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = nullptr;
PFNGLUNIFORM3FVPROC glUniform3fv = nullptr;
PFNGLUNIFORM1FPROC glUniform1f = nullptr;
PFNGLUNIFORM1IPROC glUniform1i = nullptr;
PFNGLUNIFORM2FVPROC glUniform2fv = nullptr;
PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;

namespace {

// Loads one entry point and reports which symbol failed, so a driver /
// context problem is diagnosable instead of crashing on a null call later.
template <typename T>
bool LoadOne(T& outFunc, const char* name) {
    outFunc = reinterpret_cast<T>(reinterpret_cast<void*>(glfwGetProcAddress(name)));
    if (!outFunc) {
        std::fprintf(stderr, "[GLFunctions] Failed to load OpenGL function: %s\n", name);
        return false;
    }
    return true;
}

} // namespace

bool LoadGLFunctions() {
    bool ok = true;

    ok &= LoadOne(glGenBuffers, "glGenBuffers");
    ok &= LoadOne(glBindBuffer, "glBindBuffer");
    ok &= LoadOne(glBufferData, "glBufferData");
    ok &= LoadOne(glDeleteBuffers, "glDeleteBuffers");

    ok &= LoadOne(glGenVertexArrays, "glGenVertexArrays");
    ok &= LoadOne(glBindVertexArray, "glBindVertexArray");
    ok &= LoadOne(glDeleteVertexArrays, "glDeleteVertexArrays");

    ok &= LoadOne(glEnableVertexAttribArray, "glEnableVertexAttribArray");
    ok &= LoadOne(glVertexAttribPointer, "glVertexAttribPointer");

    ok &= LoadOne(glCreateShader, "glCreateShader");
    ok &= LoadOne(glShaderSource, "glShaderSource");
    ok &= LoadOne(glCompileShader, "glCompileShader");
    ok &= LoadOne(glGetShaderiv, "glGetShaderiv");
    ok &= LoadOne(glGetShaderInfoLog, "glGetShaderInfoLog");
    ok &= LoadOne(glDeleteShader, "glDeleteShader");

    ok &= LoadOne(glCreateProgram, "glCreateProgram");
    ok &= LoadOne(glAttachShader, "glAttachShader");
    ok &= LoadOne(glLinkProgram, "glLinkProgram");
    ok &= LoadOne(glGetProgramiv, "glGetProgramiv");
    ok &= LoadOne(glGetProgramInfoLog, "glGetProgramInfoLog");
    ok &= LoadOne(glUseProgram, "glUseProgram");
    ok &= LoadOne(glDeleteProgram, "glDeleteProgram");

    ok &= LoadOne(glGetUniformLocation, "glGetUniformLocation");
    ok &= LoadOne(glUniformMatrix4fv, "glUniformMatrix4fv");
    ok &= LoadOne(glUniform3fv, "glUniform3fv");
    ok &= LoadOne(glUniform1f, "glUniform1f");
    ok &= LoadOne(glUniform1i, "glUniform1i");
    ok &= LoadOne(glUniform2fv, "glUniform2fv");
    ok &= LoadOne(glActiveTexture, "glActiveTexture");

    return ok;
}

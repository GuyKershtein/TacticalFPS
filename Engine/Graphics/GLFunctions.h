#pragma once

// Minimal OpenGL 3.3 core function loader.
//
// Windows' opengl32.lib only implements OpenGL 1.1 (glEnable, glDrawArrays,
// glClear, etc. are linked directly). Every function introduced after 1.1
// (shaders, VBOs, VAOs, uniforms) must be fetched as a function pointer from
// the driver at runtime. Rather than depending on GLAD/GLEW, we declare only
// the functions this engine actually uses and load them ourselves via
// glfwGetProcAddress once a GL context exists. Add new entries here as later
// milestones need more of the API.

#ifdef _WIN32
// Without this, Windows.h #defines min/max as macros, which silently mangle
// any later std::min/std::max/std::clamp call in any file that transitively
// includes this header (a real bug hit while adding Milestone 7's audio/
// particle code: std::max calls started failing to parse).
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif
#include <GL/gl.h>

// Types introduced after GL 1.1 that <GL/gl.h> does not define.
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

// GL enum values not present in the GL 1.1 <GL/gl.h> header.
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_RED
#define GL_RED 0x1903
#endif

// --- Function pointer typedefs (signatures match the official GL spec) -----
typedef void      (APIENTRY* PFNGLGENBUFFERSPROC)(GLsizei n, GLuint* buffers);
typedef void      (APIENTRY* PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void      (APIENTRY* PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
typedef void      (APIENTRY* PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint* buffers);

typedef void      (APIENTRY* PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint* arrays);
typedef void      (APIENTRY* PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef void      (APIENTRY* PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint* arrays);

typedef void      (APIENTRY* PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void      (APIENTRY* PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);

typedef GLuint    (APIENTRY* PFNGLCREATESHADERPROC)(GLenum type);
typedef void      (APIENTRY* PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
typedef void      (APIENTRY* PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void      (APIENTRY* PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint* params);
typedef void      (APIENTRY* PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
typedef void      (APIENTRY* PFNGLDELETESHADERPROC)(GLuint shader);

typedef GLuint    (APIENTRY* PFNGLCREATEPROGRAMPROC)(void);
typedef void      (APIENTRY* PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void      (APIENTRY* PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void      (APIENTRY* PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint* params);
typedef void      (APIENTRY* PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
typedef void      (APIENTRY* PFNGLUSEPROGRAMPROC)(GLuint program);
typedef void      (APIENTRY* PFNGLDELETEPROGRAMPROC)(GLuint program);

typedef GLint     (APIENTRY* PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar* name);
typedef void      (APIENTRY* PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
typedef void      (APIENTRY* PFNGLUNIFORM3FVPROC)(GLint location, GLsizei count, const GLfloat* value);
typedef void      (APIENTRY* PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
typedef void      (APIENTRY* PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void      (APIENTRY* PFNGLUNIFORM2FVPROC)(GLint location, GLsizei count, const GLfloat* value);
typedef void      (APIENTRY* PFNGLACTIVETEXTUREPROC)(GLenum texture);

// --- Loaded function pointers (defined in GLFunctions.cpp) ------------------
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;
extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;

extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;

extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;

extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLDELETESHADERPROC glDeleteShader;

extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;

extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
extern PFNGLUNIFORM3FVPROC glUniform3fv;
extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLUNIFORM1IPROC glUniform1i;
extern PFNGLUNIFORM2FVPROC glUniform2fv;
extern PFNGLACTIVETEXTUREPROC glActiveTexture;

// Resolves all function pointers above. Must be called once, after a GL
// context has been created and made current. Returns false (and logs which
// function failed) if any entry point is unavailable on this driver.
bool LoadGLFunctions();

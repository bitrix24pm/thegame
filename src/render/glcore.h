// glcore.h — ручная загрузка нужных функций OpenGL 3.3 (без GLEW, экономия размера)
#pragma once
#include <GL/gl.h>
#include <cstdint>

typedef char GLchar;
typedef GLuint (*PFN_glCreateShader_)(GLenum);
typedef void (*PFN_glShaderSource_)(GLuint, GLsizei, const GLchar* const*, const GLint*);
typedef void (*PFN_glCompileShader_)(GLuint);
typedef void (*PFN_glGetShaderiv_)(GLuint, GLenum, GLint*);
typedef void (*PFN_glGetShaderInfoLog_)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef GLuint (*PFN_glCreateProgram_)();
typedef void (*PFN_glAttachShader_)(GLuint, GLuint);
typedef void (*PFN_glLinkProgram_)(GLuint);
typedef void (*PFN_glUseProgram_)(GLuint);
typedef GLint (*PFN_glGetUniformLocation_)(GLuint, const GLchar*);
typedef void (*PFN_glUniformMatrix4fv_)(GLint, GLsizei, GLboolean, const GLfloat*);
typedef void (*PFN_glUniform1f_)(GLint, GLfloat);
typedef void (*PFN_glUniform2f_)(GLint, GLfloat, GLfloat);
typedef void (*PFN_glUniform3f_)(GLint, GLfloat, GLfloat, GLfloat);
typedef void (*PFN_glUniform4f_)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (*PFN_glDeleteShader_)(GLuint);
typedef void (*PFN_glGenVertexArrays_)(GLsizei, GLuint*);
typedef void (*PFN_glBindVertexArray_)(GLuint);
typedef void (*PFN_glGenBuffers_)(GLsizei, GLuint*);
typedef void (*PFN_glBindBuffer_)(GLenum, GLuint);
typedef void (*PFN_glBufferData_)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void (*PFN_glVertexAttribPointer_)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef void (*PFN_glEnableVertexAttribArray_)(GLuint);
typedef void (*PFN_glDrawArrays_)(GLenum, GLint, GLsizei);
typedef void (*PFN_glDisable_)(GLenum);
typedef void (*PFN_glUniform1i_)(GLint, GLint);

#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8
#endif
#ifndef GL_TRIANGLES
#define GL_TRIANGLES 0x0004
#endif
#ifndef GL_TRIANGLE_STRIP
#define GL_TRIANGLE_STRIP 0x0005
#endif
#ifndef GL_LINES
#define GL_LINES 0x0001
#endif
#ifndef GL_BLEND
#define GL_BLEND 0x0BE2
#endif
#ifndef GL_SRC_ALPHA
#define GL_SRC_ALPHA 0x0302
#endif
#ifndef GL_ONE_MINUS_SRC_ALPHA
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#endif

struct GLCore {
    PFN_glCreateShader_ CreateShader;
    PFN_glShaderSource_ ShaderSource;
    PFN_glCompileShader_ CompileShader;
    PFN_glGetShaderiv_ GetShaderiv;
    PFN_glGetShaderInfoLog_ GetShaderInfoLog;
    PFN_glCreateProgram_ CreateProgram;
    PFN_glAttachShader_ AttachShader;
    PFN_glLinkProgram_ LinkProgram;
    PFN_glUseProgram_ UseProgram;
    PFN_glGetUniformLocation_ GetUniformLocation;
    PFN_glUniformMatrix4fv_ UniformMatrix4fv;
    PFN_glUniform1f_ Uniform1f;
    PFN_glUniform2f_ Uniform2f;
    PFN_glUniform3f_ Uniform3f;
    PFN_glUniform4f_ Uniform4f;
    PFN_glDeleteShader_ DeleteShader;
    PFN_glGenVertexArrays_ GenVertexArrays;
    PFN_glBindVertexArray_ BindVertexArray;
    PFN_glGenBuffers_ GenBuffers;
    PFN_glBindBuffer_ BindBuffer;
    PFN_glBufferData_ BufferData;
    PFN_glVertexAttribPointer_ VertexAttribPointer;
    PFN_glEnableVertexAttribArray_ EnableVertexAttribArray;
    PFN_glDrawArrays_ DrawArrays;
    PFN_glDisable_ DisableFn;
    PFN_glUniform1i_ Uniform1i;

    bool load(void* (*getproc)(const char*));
};

extern GLCore gl;

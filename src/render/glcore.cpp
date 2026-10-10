// glcore.cpp — реализация загрузки функций GL
#include "glcore.h"
#include <cstring>

GLCore gl;

bool GLCore::load(void* (*getproc)(const char*)) {
    CreateShader = (PFN_glCreateShader_)getproc("glCreateShader");
    ShaderSource = (PFN_glShaderSource_)getproc("glShaderSource");
    CompileShader = (PFN_glCompileShader_)getproc("glCompileShader");
    GetShaderiv = (PFN_glGetShaderiv_)getproc("glGetShaderiv");
    GetShaderInfoLog = (PFN_glGetShaderInfoLog_)getproc("glGetShaderInfoLog");
    CreateProgram = (PFN_glCreateProgram_)getproc("glCreateProgram");
    AttachShader = (PFN_glAttachShader_)getproc("glAttachShader");
    LinkProgram = (PFN_glLinkProgram_)getproc("glLinkProgram");
    UseProgram = (PFN_glUseProgram_)getproc("glUseProgram");
    GetUniformLocation = (PFN_glGetUniformLocation_)getproc("glGetUniformLocation");
    UniformMatrix4fv = (PFN_glUniformMatrix4fv_)getproc("glUniformMatrix4fv");
    Uniform1f = (PFN_glUniform1f_)getproc("glUniform1f");
    Uniform2f = (PFN_glUniform2f_)getproc("glUniform2f");
    Uniform3f = (PFN_glUniform3f_)getproc("glUniform3f");
    Uniform4f = (PFN_glUniform4f_)getproc("glUniform4f");
    DeleteShader = (PFN_glDeleteShader_)getproc("glDeleteShader");
    GenVertexArrays = (PFN_glGenVertexArrays_)getproc("glGenVertexArrays");
    BindVertexArray = (PFN_glBindVertexArray_)getproc("glBindVertexArray");
    GenBuffers = (PFN_glGenBuffers_)getproc("glGenBuffers");
    BindBuffer = (PFN_glBindBuffer_)getproc("glBindBuffer");
    BufferData = (PFN_glBufferData_)getproc("glBufferData");
    VertexAttribPointer = (PFN_glVertexAttribPointer_)getproc("glVertexAttribPointer");
    EnableVertexAttribArray = (PFN_glEnableVertexAttribArray_)getproc("glEnableVertexAttribArray");
    DrawArrays = (PFN_glDrawArrays_)getproc("glDrawArrays");
    DisableFn = (PFN_glDisable_)getproc("glDisable");
    return CreateShader && ShaderSource && CompileShader && GetShaderiv && GetShaderInfoLog
        && CreateProgram && AttachShader && LinkProgram && UseProgram && GetUniformLocation
        && UniformMatrix4fv && Uniform1f && Uniform2f && Uniform3f && Uniform4f && DeleteShader
        && GenVertexArrays && BindVertexArray && GenBuffers && BindBuffer && BufferData
        && VertexAttribPointer && EnableVertexAttribArray && DrawArrays && DisableFn;
}

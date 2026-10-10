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
    glGenVertexArrays = (PFN_glGenVertexArrays_)getproc("glGenVertexArrays");
    glBindVertexArray = (PFN_glBindVertexArray_)getproc("glBindVertexArray");
    glGenBuffers = (PFN_glGenBuffers_)getproc("glGenBuffers");
    glBindBuffer = (PFN_glBindBuffer_)getproc("glBindBuffer");
    glBufferData = (PFN_glBufferData_)getproc("glBufferData");
    glVertexAttribPointer = (PFN_glVertexAttribPointer_)getproc("glVertexAttribPointer");
    glEnableVertexAttribArray = (PFN_glEnableVertexAttribArray_)getproc("glEnableVertexAttribArray");
    glDrawArrays = (PFN_glDrawArrays_)getproc("glDrawArrays");
    glDisableFn = (PFN_glDisable_)getproc("glDisable");
    glUniform1i = (PFN_glUniform1i_)getproc("glUniform1i");
    return glCreateShader && glShaderSource && glCompileShader && glGetShaderiv && glGetShaderInfoLog
        && glCreateProgram && glAttachShader && glLinkProgram && glUseProgram && glGetUniformLocation
        && glUniformMatrix4fv && glUniform1f && glUniform2f && glUniform3f && glUniform4f && glDeleteShader
        && glGenVertexArrays && glBindVertexArray && glGenBuffers && glBindBuffer && glBufferData
        && glVertexAttribPointer && glEnableVertexAttribArray && glDrawArrays && glDisableFn && glUniform1i;
}

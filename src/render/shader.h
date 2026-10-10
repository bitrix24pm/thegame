// shader.h/cpp — компиляция шейдеров из строк в памяти (никаких файлов-ассетов)
#pragma once
#include "glcore.h"
#include <cstdio>

struct Shader {
    GLuint prog = 0;

    bool compile(const char* vsrc, const char* fsrc) {
        GLuint vs = gl.CreateShader(GL_VERTEX_SHADER);
        gl.ShaderSource(vs, 1, &vsrc, nullptr);
        gl.CompileShader(vs);
        GLint ok = 0;
        gl.GetShaderiv(vs, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024];
            gl.GetShaderInfoLog(vs, sizeof(log), nullptr, log);
            printf("[shader] VS error: %s\n", log);
            return false;
        }
        GLuint fs = gl.CreateShader(GL_FRAGMENT_SHADER);
        gl.ShaderSource(fs, 1, &fsrc, nullptr);
        gl.CompileShader(fs);
        gl.GetShaderiv(fs, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024];
            gl.GetShaderInfoLog(fs, sizeof(log), nullptr, log);
            printf("[shader] FS error: %s\n", log);
            return false;
        }
        prog = gl.CreateProgram();
        gl.AttachShader(prog, vs);
        gl.AttachShader(prog, fs);
        gl.LinkProgram(prog);
        gl.GetShaderiv(prog, GL_LINK_STATUS, &ok);
        if (!ok) { printf("[shader] link failed\n"); return false; }
        gl.DeleteShader(vs);
        gl.DeleteShader(fs);
        return true;
    }

    void use() const { gl.UseProgram(prog); }
    GLint loc(const char* n) const { return gl.GetUniformLocation(prog, n); }
};

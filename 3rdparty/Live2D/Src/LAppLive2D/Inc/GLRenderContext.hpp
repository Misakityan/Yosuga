//
// Created by misaki on 2026/6/23.
//

#pragma once
#include "IRenderContext.hpp"
#include "LAppOpenGL.hpp"

class GLRenderContext final : public IRenderContext {
public:
    void Clear(float r, float g, float b, float a) override {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void ClearDepth(float depth) override {
        LAPP_GL_CLEAR_DEPTH(depth);
    }

    void SetViewport(int x, int y, int w, int h) override {
        glViewport(x, y, w, h);
    }

    uintptr_t CreateShaderProgram() override {
        return CompileShader();
    }

    uintptr_t GetShaderProgram() const override {
        return _programId;
    }

    void InitializeGLState() override {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

private:
    GLuint _programId = 0;

    GLuint CompileShader() {
        if (_programId) return _programId;

#if defined(QT_OPENGL_ES_2) || defined(QT_OPENGL_ES_3) || defined(EMBEDDED_LINUX)
        const char* vertexShader =
            "#version 100\n"
            "attribute vec3 position;\n"
            "attribute vec2 uv;\n"
            "varying vec2 vuv;\n"
            "void main() {\n"
            "    gl_Position = vec4(position, 1.0);\n"
            "    vuv = uv;\n"
            "}\n";
        const char* fragmentShader =
            "#version 100\n"
            "precision mediump float;\n"
            "varying vec2 vuv;\n"
            "uniform sampler2D texture;\n"
            "uniform vec4 baseColor;\n"
            "void main() {\n"
            "    gl_FragColor = texture2D(texture, vuv) * baseColor;\n"
            "}\n";
#else
        const char* vertexShader =
            "#version 120\n"
            "attribute vec3 position;\n"
            "attribute vec2 uv;\n"
            "varying vec2 vuv;\n"
            "void main(void) {\n"
            "    gl_Position = vec4(position, 1.0);\n"
            "    vuv = uv;\n"
            "}\n";
        const char* fragmentShader =
            "#version 120\n"
            "varying vec2 vuv;\n"
            "uniform sampler2D texture;\n"
            "uniform vec4 baseColor;\n"
            "void main(void) {\n"
            "    gl_FragColor = texture2D(texture, vuv) * baseColor;\n"
            "}\n";
#endif

        GLuint vertId = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertId, 1, &vertexShader, nullptr);
        glCompileShader(vertId);

        GLuint fragId = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragId, 1, &fragmentShader, nullptr);
        glCompileShader(fragId);

        _programId = glCreateProgram();
        glAttachShader(_programId, vertId);
        glAttachShader(_programId, fragId);
        glLinkProgram(_programId);
        glUseProgram(_programId);

        glDeleteShader(vertId);
        glDeleteShader(fragId);
        return _programId;
    }
};
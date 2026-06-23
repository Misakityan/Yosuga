//
// Created by misaki on 2026/6/23.
//
#pragma once
#include <cstdint>

class IRenderContext {
public:
    virtual ~IRenderContext() = default;

    virtual void Clear(float r, float g, float b, float a) = 0;
    virtual void ClearDepth(float depth) = 0;
    virtual void SetViewport(int x, int y, int w, int h) = 0;
    virtual uintptr_t CreateShaderProgram() = 0;
    virtual uintptr_t GetShaderProgram() const = 0;
    virtual void InitializeGLState() = 0;
};
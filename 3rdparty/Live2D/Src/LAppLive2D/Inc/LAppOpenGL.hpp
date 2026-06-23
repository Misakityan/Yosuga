//
// Created by misaki on 2026/5/13.
//

// LAppOpenGL.hpp
#pragma once

// 根据是否嵌入式选择 OpenGL 头
#if !defined(EMBEDDED_LINUX)
    // 桌面 OpenGL
    #include <GL/glew.h>
    #include <GLFW/glfw3.h>
#else
    // 嵌入式 OpenGL ES
    #include <EGL/egl.h>
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
#endif

// 统一深度清除函数
#if defined(EMBEDDED_LINUX)
    #define LAPP_GL_CLEAR_DEPTH(d) glClearDepthf(d)
#else
    #define LAPP_GL_CLEAR_DEPTH(d) glClearDepth(d)
#endif

#define CONCAT_IMPL(a, b) a##b

#define CONCAT(a, b) CONCAT_IMPL(a, b)

// 渲染后端编译期类型选择 与 Cubism SDK 的 CubismRenderer::Create() 条件编译对齐
#if defined(RENDER_BACKEND_VULKAN)
  #define RENDERER_BACKEND_TAG Vulkan
#elif defined(RENDER_BACKEND_D3D11)
  #define RENDERER_BACKEND_TAG D3D11
#elif defined(RENDER_BACKEND_D3D9)
  #define RENDERER_BACKEND_TAG D3D9
#elif defined(RENDER_BACKEND_METAL)
  #define RENDERER_BACKEND_TAG Metal
#else
  #define RENDERER_BACKEND_TAG OpenGLES2
#endif

// 类型别名宏，用于 Csm 命名空间下的后端特定类型
#define CUBISM_RENDERER_TYPE   CONCAT(Csm::Rendering::CubismRenderer_, RENDERER_BACKEND_TAG)
#define CUBISM_OFFSCREEN_TYPE  CONCAT(Csm::Rendering::CubismOffscreenSurface_, RENDERER_BACKEND_TAG)
//
// Created by misaki on 2026/6/23.
//

#pragma once
#include <cstdint>

class ISpriteRenderer {
public:
    virtual ~ISpriteRenderer() = default;

    virtual void SetColor(float r, float g, float b, float a) = 0;
    virtual void SetWindowSize(int w, int h) = 0;
    virtual void RenderImmidiate(uintptr_t textureId,
                                 const float uvVertex[8]) const = 0;
    virtual bool IsHit(float px, float py) const = 0;
    virtual void ResetRect(float x, float y, float w, float h) = 0;
    virtual uintptr_t GetTextureId() const = 0;
};

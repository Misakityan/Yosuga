/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include "LAppOpenGL.hpp"
#include "ISpriteRenderer.hpp"     // 渲染后端抽象

/**
* @brief スプライトを実装するクラス。
*
* テクスチャID、Rectの管理。
*
*/
// class LAppSprite                                 // 原
class LAppSprite : public ISpriteRenderer           // 继承渲染抽象接口
{
public:
    /**
    * @brief Rect 構造体。
    */
    struct Rect
    {
    public:
        float left;     ///< 左辺
        float right;    ///< 右辺
        float up;       ///< 上辺
        float down;     ///< 下辺
    };

    /**
    * @brief コンストラクタ
    *
    * @param[in]       x            x座標
    * @param[in]       y            y座標
    * @param[in]       width        横幅
    * @param[in]       height       高さ
    * @param[in]       textureId    テクスチャID
    * @param[in]       programId    シェーダID
    */
    // LAppSprite(float x, float y, float width, float height, GLuint textureId, GLuint programId);  // 原
    LAppSprite(float x, float y, float width, float height, uintptr_t textureId, uintptr_t programId); // 抽象类型

    /**
    * @brief デストラクタ
    */
    ~LAppSprite();

    /**
    * @brief Getter テクスチャID
    * @return テクスチャIDを返す
    */
    // GLuint GetTextureId() { return _textureId; }       // 原
    uintptr_t GetTextureId() const override { return _textureId; } // 抽象类型

    /**
    * @brief 描画する
    *
    */
    void Render() const;

    /**
    * @brief テクスチャIDを指定して描画する
    *
    */
    // void RenderImmidiate(GLuint textureId, const GLfloat uvVertex[8]) const;   // 原
    void RenderImmidiate(uintptr_t textureId, const float uvVertex[8]) const override; // 抽象类型

    /**
    * @brief コンストラクタ
    *
    * @param[in]       pointX    x座標
    * @param[in]       pointY    y座標
    */
    // bool IsHit(float pointX, float pointY) const;     // 原
    bool IsHit(float pointX, float pointY) const override; // 接口实现

    /**
     * @brief 色設定
     *
     * @param[in]       r (0.0~1.0)
     * @param[in]       g (0.0~1.0)
     * @param[in]       b (0.0~1.0)
     * @param[in]       a (0.0~1.0)
     */
    // void SetColor(float r, float g, float b, float a);      // 原
    void SetColor(float r, float g, float b, float a) override; // 接口实现

    /**
     * @brief サイズ再設定
     *
     * @param[in]       x            x座標
     * @param[in]       y            y座標
     * @param[in]       width        横幅
     * @param[in]       height       高さ
     */
    // void ResetRect(float x, float y, float width, float height);     // 原
    void ResetRect(float x, float y, float width, float height) override; // 接口实现

    /**
     * @brief ウインドウサイズ設定
     *
     * @param[in]       width        横幅
     * @param[in]       height       高さ
     */
    // void SetWindowSize(int width, int height);     // 原
    void SetWindowSize(int width, int height) override; // 接口实现

private:
    // GLuint _textureId;      ///< テクスチャID  原
    uintptr_t _textureId;       ///< テクスチャID  抽象类型
    Rect _rect;             ///< 矩形
    int _positionLocation;  ///< 位置アトリビュート
    int _uvLocation;        ///< UVアトリビュート
    int _textureLocation;   ///< テクスチャアトリビュート
    int _colorLocation;     ///< カラーアトリビュート

    float _spriteColor[4];  ///< 表示カラー
    int _maxWidth;  ///< ウインドウ幅
    int _maxHeight;  ///< ウインドウ高さ
};


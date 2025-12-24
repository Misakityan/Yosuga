//
// Created by Administrator on 2025/4/1.
//

/**
 * @brief 模型页面
 * 暂时只做最简单功能切换模型
 */
#pragma once

#include "BasePage.h"
#include "ElaPushButton.h"
#include "ElaLineEdit.h"
#include "ElaComboBox.h"
#include "ElaSlider.h"

#include <QUrl>
#include <utility>

class ModelPage final : public BasePage
{
Q_OBJECT
public:
    Q_INVOKABLE explicit ModelPage(QWidget* parent = nullptr);

    std::pair<QString, QString> splitPath(const QString& fullPath);

    ~ModelPage() override;


private:
    // 设置当前模型
    ElaLineEdit* modelUrlEdit = nullptr;            /// 模型Url 编辑框
    ElaPushButton* modelChoosePushButton = nullptr; /// 选择模型按钮
    ElaPushButton* modelUsePushButton = nullptr;    /// 使用模型按钮
    ElaSlider* modelSlider = nullptr;               /// 滑块(用于设置模型实时大小)

    QUrl modelFileUrl;
    QString modelFilePathFirst;
    QString modelFilePathSecond;
};

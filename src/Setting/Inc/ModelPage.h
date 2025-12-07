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

#include <QUrl>
#include <utility>

class ElaLineEdit;
class ElaPushButton;
class ModelPage final : public BasePage
{
Q_OBJECT
public:
    Q_INVOKABLE explicit ModelPage(QWidget* parent = nullptr);

    std::pair<QString, QString> splitPath(const QString& fullPath);

    ~ModelPage() override;


private:
    // 设置当前模型
    ElaLineEdit* modelUrlEdit = nullptr;
    ElaPushButton* modelChoosePushButton = nullptr;
    ElaPushButton* modelUsePushButton = nullptr;
    QUrl modelFileUrl;
    QString modelFilePathFirst;
    QString modelFilePathSecond;
};

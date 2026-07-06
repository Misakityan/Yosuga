#ifdef EMBEDDED_LINUX
#include "PttButton.hpp"
#include <QIcon>
#include <algorithm>

PttButton::PttButton(QWidget* parent)
    : QPushButton(parent)
{
    setIcon(QIcon("Resources/Pic/Others/voice.png"));
    setFlat(true);
    raise();
    show();

    connect(this, &QPushButton::pressed,  this, &PttButton::pressedForPtt);
    connect(this, &QPushButton::released, this, &PttButton::releasedForPtt);
}

void PttButton::updateLayout(int parentWidth, int parentHeight)
{
    if (parentWidth <= 0 || parentHeight <= 0) return;

    const int minDim = std::min(parentWidth, parentHeight);
    const int btnSize = std::clamp(static_cast<int>(minDim * scale), minSize, maxSize);
    const int x = parentWidth  - btnSize - static_cast<int>(parentWidth  * rightMargin);
    const int y = parentHeight - btnSize - static_cast<int>(parentHeight * bottomMargin);
    const int iconPad  = btnSize / 8;

    setFixedSize(btnSize, btnSize);
    move(x, y);
    setIconSize(QSize(btnSize - iconPad * 2, btnSize - iconPad * 2));
    applyStyleSheet(btnSize);
}

void PttButton::applyStyleSheet(int btnSize)
{
    setStyleSheet(
        QString(
            "QPushButton {"
            "  background-color: rgba(255, 255, 255, 60);"
            "  border-radius: %1px;"
            "  border: none;"
            "}"
            "QPushButton:pressed {"
            "  background-color: rgba(250, 80, 80, 150);"
            "}"
        ).arg(btnSize / 2)
    );
}
#endif

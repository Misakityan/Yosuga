#pragma once

#ifdef EMBEDDED_LINUX

#include <QPushButton>

class PttButton : public QPushButton
{
    Q_OBJECT
public:
    explicit PttButton(QWidget* parent = nullptr);

    void updateLayout(int parentWidth, int parentHeight);

    float scale        = 0.15f;
    float rightMargin  = 0.05f;
    float bottomMargin = 0.05f;
    int   minSize      = 48;
    int   maxSize      = 128;

signals:
    void pressedForPtt();
    void releasedForPtt();

private:
    void applyStyleSheet(int btnSize);
};

#endif

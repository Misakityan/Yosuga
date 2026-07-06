#pragma once

#ifdef YOSUGA_DEBUG

#include <QElapsedTimer>
#include <QString>

class QTimer;

class FpsOverlay
{
public:
    FpsOverlay();
    ~FpsOverlay();

    void tick();
    void draw(int parentWidth, int parentHeight);

    float rightMargin = 0.03f;
    float topMargin   = 0.03f;
    int   fontSize    = 20;

private:
    QElapsedTimer _elapsed;
    QTimer* _timer = nullptr;
    int _frameCount   = 0;
    int _displayedFps = 0;
    QString _text = "FPS: --";

    void recalcFps();
};

#endif

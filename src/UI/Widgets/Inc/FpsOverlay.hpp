#pragma once
#ifdef YOSUGA_DEBUG

#include <QLabel>

class QTimer;
class QElapsedTimer;

class FpsOverlay : public QLabel
{
    Q_OBJECT
public:
    explicit FpsOverlay(QWidget* parent = nullptr);
    ~FpsOverlay();

    void tick();
    void renderToPixmap();  // 离线渲染文字到 pixmap


    float rightMargin = 0.03f;
    float topMargin   = 0.03f;
    int   fontSize    = 18;

private:
    QTimer* _timer = nullptr;
    QElapsedTimer* _elapsed = nullptr;
    int _frameCount   = 0;
    int _displayedFps = 0;

    void recalcFps();
};

#endif
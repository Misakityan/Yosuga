#ifdef YOSUGA_DEBUG
#include "FpsOverlay.hpp"
#include <QPainter>
#include <QOpenGLPaintDevice>
#include <QTimer>
#include <cstdio>

FpsOverlay::FpsOverlay()
{
    _timer = new QTimer;
    _timer->setTimerType(Qt::PreciseTimer);
    QObject::connect(_timer, &QTimer::timeout, [this]() { recalcFps(); });
    _timer->start(1000);
    _elapsed.start();
}

FpsOverlay::~FpsOverlay()
{
    delete _timer;
}

void FpsOverlay::tick()
{
    ++_frameCount;
}

void FpsOverlay::draw(int parentWidth, int parentHeight)
{
    QOpenGLPaintDevice device(QSize(parentWidth, parentHeight));
    QPainter painter(&device);
    painter.setRenderHint(QPainter::Antialiasing);

    QFont f;
    f.setPixelSize(fontSize);
    f.setBold(true);
    f.setStyleHint(QFont::Monospace);

    QFontMetrics fm(f);
    const int pad = 8;
    const int textW = fm.horizontalAdvance(_text) + pad * 2;
    const int textH = fm.height() + pad;
    const int x = parentWidth  - textW - static_cast<int>(parentWidth  * rightMargin);
    const int y = static_cast<int>(parentHeight * topMargin);

    painter.setBrush(QColor(0, 0, 0, 180));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(x, y, textW, textH, 4, 4);

    painter.setFont(f);
    const QColor fg = _displayedFps <= 0   ? QColor(170, 170, 170) :
                      _displayedFps <= 30  ? QColor(255,  68,  68) :
                      _displayedFps <= 45  ? QColor(255, 170,   0) :
                                             QColor( 68, 255,  68);
    painter.setPen(fg);
    painter.drawText(QRect(x + pad, y, textW - pad * 2, textH),
                     Qt::AlignVCenter | Qt::AlignLeft, _text);
}

void FpsOverlay::recalcFps()
{
    const qint64 ms = _elapsed.elapsed();
    if (ms == 0) return;

    _displayedFps = static_cast<int>(_frameCount * 1000.0 / ms);
    _frameCount  = 0;
    _elapsed.restart();

    _text = QString("FPS: %1").arg(_displayedFps);
    fprintf(stderr, "[FPS] %s\n", _text.toUtf8().constData());
}
#endif

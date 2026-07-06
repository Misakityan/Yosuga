#ifdef YOSUGA_DEBUG
#include "FpsOverlay.hpp"
#include <QTimer>
#include <QElapsedTimer>
#include <QFont>
#include <QFontMetrics>
#include <QFontDatabase>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QFile>
#include <cstdio>

FpsOverlay::FpsOverlay(QWidget* parent)
    : QLabel(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAlignment(Qt::AlignCenter);
    raise();
    show();

    // 尝试加载系统字体
    QStringList fontPaths = {
        "/usr/share/fonts/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/noto/NotoSansCJKsc-Regular.otf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
    };

    bool loaded = false;
    for (const QString& path : fontPaths) {
        if (QFile::exists(path)) {
            int id = QFontDatabase::addApplicationFont(path);
            if (id != -1) {
                QString family = QFontDatabase::applicationFontFamilies(id).first();
                QFont f(family, fontSize);
                f.setBold(true);
                setFont(f);
                fprintf(stderr, "[FpsOverlay] Loaded font: %s from %s\n",
                        family.toUtf8().constData(), path.toUtf8().constData());
                loaded = true;
                break;
            } else {
                fprintf(stderr, "[FpsOverlay] addApplicationFont failed: %s\n",
                        path.toUtf8().constData());
            }
        }
    }
    if (!loaded) {
        // fallback：不指定字体族，用 Qt 默认
        QFont f = font();
        f.setPointSize(fontSize);
        f.setBold(true);
        setFont(f);
        fprintf(stderr, "[FpsOverlay] WARNING: No font loaded, using default: %s\n",
                font().family().toUtf8().constData());
    }

    // 先显示初始帧
    _displayedFps = 0;
    renderToPixmap();

    _elapsed = new QElapsedTimer;
    _elapsed->start();

    _timer = new QTimer(this);
    _timer->setTimerType(Qt::PreciseTimer);
    connect(_timer, &QTimer::timeout, this, &FpsOverlay::recalcFps);
    _timer->start(1000);
}

FpsOverlay::~FpsOverlay()
{
    delete _elapsed;
}

void FpsOverlay::tick()
{
    ++_frameCount;
}

void FpsOverlay::renderToPixmap()
{
    QString text = QString("FPS: %1").arg(_displayedFps);

    // 计算尺寸
    QFontMetrics fm(font());
    const int padX = 12;
    const int padY = 8;
    const int textW = fm.horizontalAdvance(text);
    const int textH = fm.height();
    const int w = textW + padX * 2;
    const int h = textH + padY * 2;

    // 父窗口大小决定位置
    int parentW = parentWidget() ? parentWidget()->width() : 1080;
    int parentH = parentWidget() ? parentWidget()->height() : 1920;
    const int x = parentW - w - static_cast<int>(parentW * rightMargin);
    const int y = static_cast<int>(parentH * topMargin);

    // 设置几何位置
    setGeometry(x, y, w, h);

    // 用 QImage 离线渲染，不依赖 QLabel 的字体系统
    QImage img(w, h, QImage::Format_ARGB32);
    img.fill(Qt::transparent);      // 透明

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);

    // 文字颜色：根据 FPS 变色
    const QColor fg = _displayedFps <= 0  ? QColor(170, 170, 170) :
                      _displayedFps <= 30 ? QColor(255,  68,  68) :
                      _displayedFps <= 45 ? QColor(255, 170,   0) :
                                            QColor( 68, 255,  68);
    p.setPen(fg);
    p.setFont(font());
    p.drawText(img.rect(), Qt::AlignCenter, text);
    p.end();

    setPixmap(QPixmap::fromImage(img));
}

void FpsOverlay::recalcFps()
{
    const qint64 ms = _elapsed->elapsed();
    if (ms == 0) return;

    _displayedFps = static_cast<int>(_frameCount * 1000.0 / ms);
    _frameCount = 0;
    _elapsed->restart();

    // fprintf(stderr, "[FPS] %d\n", _displayedFps);

    // 重新渲染为 pixmap
    renderToPixmap();
}

#endif
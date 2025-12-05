#include <QtWidgets/QApplication>
#include "ElaApplication.h"
#include <QFontDatabase>
#include <QMessageBox>
#include "GLCore.h"

#ifdef Q_OS_UNIX
#include <QLoggingCategory>
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    eApp->init();
    // 设置云母效果图片
    eApp->setMicaImagePath("Resources/Pic/Others/MicaBase.png");
#ifdef Q_OS_UNIX
    // 设置日志过滤规则，将这个特定分类的警告级别设置为“关闭”
    QLoggingCategory::setFilterRules("qt.multimedia.ffmpeg.libsymbolsresolver.warning=false");
#endif
    GLCore w(360, 480);     // 创建默认的窗口
    w.show();
    return QApplication::exec();
}

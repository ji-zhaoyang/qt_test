#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QStringList>
#include <QWebEngineSettings>
#include <stdlib.h>

namespace
{
QFont createGlobalUiFont()
{
    const QStringList preferredFamilies = {"Noto Sans CJK SC", "Noto Sans SC", "Source Han Sans SC",
                                           "WenQuanYi Micro Hei", "Microsoft YaHei", "SimHei"};
    const QStringList installedFamilies = QFontDatabase().families();

    for (const QString &family : preferredFamilies)
    {
        if (installedFamilies.contains(family))
        {
            QFont font(family, 10);
            font.setStyleStrategy(QFont::PreferAntialias);
            return font;
        }
    }

    QFont fallbackFont;
    fallbackFont.setPointSize(10);
    fallbackFont.setStyleStrategy(QFont::PreferAntialias);
    return fallbackFont;
}
} // namespace

int main(int argc, char *argv[])
{
    // 【终极排查】：将 DISPLAY 环境变量强行注入到当前进程以及所有子进程中
    // qputenv 有时在 Linux 下对底层 X11/GLX 库不生效，必须用原生 setenv
    setenv("DISPLAY", ":0", 1);
    qDebug() << "[INIT] 强制设置环境变量 DISPLAY=" << qgetenv("DISPLAY");

    // 关闭沙箱以防止在 Linux root 权限下运行 WebEngine 报错
    qputenv("QTWEBENGINE_DISABLE_SANDBOX", "1");

    QApplication app(argc, argv);

    QFont uiFont = createGlobalUiFont();
    app.setFont(uiFont);
    qDebug() << "[INIT] 全局主字体已设置为:" << uiFont.family();

    // 加载全局的 QSS 主题文件（暗黑风格）
    // 语法和 Web 端的 CSS 非常像
    QString qss = R"(
        QMainWindow { background-color: #1e1e1e; }
        QPushButton { 
            background-color: #0078D7; 
            color: white; 
            border-radius: 4px;
            padding: 10px;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #005A9E; }
    )";
    // 将样式应用到整个软件的所有窗口
    app.setStyleSheet(qss);

    // 启动主窗口
    qDebug() << "[INIT] 准备实例化 MainWindow...";
    MainWindow w;
    qDebug() << "[INIT] MainWindow 实例化成功！准备显示...";

    // 【修改为真正的全屏】：覆盖掉 Ubuntu/Jetson 的系统侧边栏和顶栏
    w.showFullScreen();

    qDebug() << "[INIT] MainWindow 显示成功！即将进入事件循环...";

    return app.exec(); // 死循环，也就是事件循环（接管了 C 语言的 while(1)）
}

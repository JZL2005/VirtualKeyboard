#include <QApplication>           // Qt 应用程序类
#include "virtualkeyboardwidget.h" // 包含虚拟键盘窗口类
#include <QStyleFactory> // 包含样式工厂

int main(int argc, char *argv[]) {
    // 创建 Qt 应用程序实例
    QApplication a(argc, argv);

    // 推荐设置一个融合样式，确保跨平台视觉一致性
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // 创建虚拟键盘窗口实例
    VirtualKeyboardWidget keyboard;
    // 显示虚拟键盘窗口
    keyboard.show();

    // 进入 Qt 应用程序的事件循环
    return QApplication::exec();
}
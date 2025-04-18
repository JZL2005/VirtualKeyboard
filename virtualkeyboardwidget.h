#ifndef VIRTUALKEYBOARD_VIRTUALKEYBOARDWIDGET_H
#define VIRTUALKEYBOARD_VIRTUALKEYBOARDWIDGET_H

#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QList>
#include <QMap>
#include "keyboardlayout.h" // 包含键盘布局定义

// --- Windows API 类型前向声明 ---
// 避免在头文件中包含庞大的 windows.h
#ifdef _WIN32
struct HWND__; typedef HWND__* HWND;     // 窗口句柄
struct tagINPUT; typedef tagINPUT INPUT; // SendInput 函数需要的结构体
#endif

// 主虚拟键盘窗口类
class VirtualKeyboardWidget : public QWidget {
Q_OBJECT // Qt 元对象系统宏

public:
    // 构造函数
    explicit VirtualKeyboardWidget(QWidget *parent = nullptr);
    // 析构函数 (使用默认实现)
    ~VirtualKeyboardWidget() override = default;

protected:
    // 重写窗口尺寸改变事件处理函数
    void resizeEvent(QResizeEvent *event) override;

// 私有槽函数，响应信号
private slots:
    void onKeyPressed();        // 按键按下时调用
    void onKeyReleased();       // 按键释放时调用
    void changeOpacity(int value); // 透明度滑块值改变时调用
    void positionWindow();      // 定位窗口到屏幕底部

// 私有成员函数
private:
    void setupUI();             // 初始化用户界面元素
    // 创建键盘布局 (将 KeyInfo 转换为 QPushButton)
    void createKeyboardLayout(QWidget* parentWidget, QGridLayout* layout, const KeyboardLayout& keyRows);
    void updateModifierKeysVisuals(); // 更新修饰键 (Shift, Ctrl, Alt, Caps...) 的视觉状态 (文本大小写, 按钮样式)
    // 使用 Windows API 模拟按键事件
    void simulateKey(int vkCode, int scanCode, bool press, bool isExtended);
    // SendInput API 的包装函数，包含日志记录
    void sendInputWrapper(INPUT input);

    // --- UI 元素指针 ---
    QVBoxLayout *outerLayout;       // 最外层垂直布局 (包含键盘和滑块)
    QHBoxLayout *keyboardLayout;    // 包含左右键盘区域的水平布局
    QWidget *leftKeyboardWidget;    // 左键盘容器 QWidget
    QWidget *rightKeyboardWidget;   // 右键盘容器 QWidget
    QGridLayout *leftGridLayout;    // 左键盘网格布局
    QGridLayout *rightGridLayout;   // 右键盘网格布局
    QSlider *opacitySlider;         // 透明度调节滑块
    QList<QPushButton*> keyButtons; // 存储所有按键按钮的指针，方便统一处理

    // --- 键盘状态标志 ---
    // 这些标志跟踪修饰键的当前“按下”状态
    bool shiftActive = false;
    bool ctrlActive = false;
    bool altActive = false;
    bool winActive = false;
    // 这些标志跟踪切换键的“激活”状态
    bool capsLockActive = false;
    bool numLockActive = false;
    bool scrollLockActive = false;

    // --- 布局数据 ---
    KeyboardLayout fullLayoutData;  // 完整的键盘布局数据
    KeyboardLayout leftLayoutData;  // 左半部分键盘布局数据
    KeyboardLayout rightLayoutData; // 右半部分键盘布局数据
};

#endif // VIRTUALKEYBOARD_VIRTUALKEYBOARDWIDGET_H
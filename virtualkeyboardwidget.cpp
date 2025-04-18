#include "virtualkeyboardwidget.h"
#include "keyboardlayout.h"

#include <QScreen>
#include <QGuiApplication>
#include <QApplication>
#include <QStyle>
#include <QFile>
#include <QDebug>
#include <QResizeEvent>
#include <QRegularExpression> // 用于在 QSS 中替换透明度值

// --- Windows API 头文件 ---
#ifdef _WIN32
// WIN32_LEAN_AND_MEAN 和 NOMINMAX 由 CMakeLists.txt 定义
#include <windows.h>
#include <winuser.h> // 包含 SendInput, GetKeyState, INPUT, KEYEVENTF_*, GetForegroundWindow 等定义
#endif

// --- 常量定义 ---
const double MIN_OPACITY = 0.2; // 最小透明度 (20%)
const double MAX_OPACITY = 1.0; // 最大透明度 (100%)
const int DEFAULT_OPACITY_PERCENT = 85; // 默认透明度百分比
const int KEY_MIN_HEIGHT = 45; // 按键最小高度 (像素)
const int KEY_MIN_WIDTH = 45;  // 按键最小宽度 (像素)
const int AUTO_REPEAT_DELAY_MS = 500; // 按键自动重复的初始延迟 (毫秒)

// --- 构造函数 ---
VirtualKeyboardWidget::VirtualKeyboardWidget(QWidget *parent)
        : QWidget(parent)
{
    // --- 窗口设置 ---
    // 设置窗口标志:
    // Qt::WindowStaysOnTopHint: 窗口始终保持在顶层
    // Qt::FramelessWindowHint: 无边框窗口 (没有标题栏、边框)
    // Qt::Tool: 工具窗口类型，通常不在任务栏显示，不参与 Alt+Tab 切换
    // Qt::WindowDoesNotAcceptFocus: 窗口本身不接受键盘焦点 (关键！防止键盘自身获取焦点)
    setWindowFlags(Qt::WindowStaysOnTopHint |
                   Qt::FramelessWindowHint |
                   Qt::Tool |
                   Qt::WindowDoesNotAcceptFocus);

    // 启用窗口部件的透明背景绘制
    setAttribute(Qt::WA_TranslucentBackground);
    // 告诉 Qt 不要绘制系统默认背景，因为我们将通过 QSS 或子部件自行绘制
    setAttribute(Qt::WA_NoSystemBackground, true);
    // 主窗口本身完全不透明，透明效果由子部件背景或 QSS 的 rgba 实现
    setWindowOpacity(1.0);
    // 设置窗口标题 (虽然无边框，但可能在某些地方显示)
    setWindowTitle("虚拟键盘");

    // --- 样式设置 (QSS) ---
    // 使用 QStringLiteral 避免编码问题，%1 %2 %3 是占位符
    setStyleSheet(QStringLiteral(R"(
        /* 主窗口透明 */
        QWidget { background-color: transparent; color: white; }
        /* 键盘半区样式 */
        QWidget#KeyboardHalf {
             /* 背景色: 深灰色，带 alpha 透明度 (由 %1 控制) */
             background-color: rgba(40, 40, 45, %1);
             border-radius: 8px; /* 圆角 */
             border: 1px solid rgba(80, 80, 80, %1); /* 边框，带 alpha */
             padding: 4px; /* 内边距 */
        }
        /* 按钮通用样式 */
        QPushButton {
            /* 背景渐变 */
            background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #5a5a5a, stop: 1 #3a3a3a);
            color: white; /* 文字颜色 */
            border: 1px solid #666666; /* 边框 */
            border-radius: 5px; /* 圆角 */
            padding: 5px; /* 内边距 */
            min-height: %2px; /* 最小高度 (由 %2 控制) */
            min-width: %3px;  /* 最小宽度 (由 %3 控制) */
            font-size: 11pt; /* 字体大小 */
            font-weight: bold; /* 字体加粗 */
        }
        /* 按钮按下时的样式 */
        QPushButton:pressed {
            background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #007ACC, stop: 1 #005C99); /* 蓝色渐变 */
            border-color: #00AACC;
        }
        /* 修饰键 (Shift, Ctrl, Alt, Win) 按下时的样式 (通过 objectName 设置) */
        QPushButton#ModifierActive {
             background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #007ACC, stop: 1 #005C99); /* 蓝色 */
             border: 1px solid #00AACC;
         }
        /* 切换键 (Caps Lock等) 激活时的样式 (通过 objectName 设置) */
        QPushButton#ToggleActive {
              background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #50A64F, stop: 1 #388E3C); /* 绿色 */
              border: 1px solid #81C784;
        }
        /* 特殊功能键的默认样式 (Enter, Backspace 等) */
        QPushButton#SpecialKey {
             background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #686868, stop: 1 #484848); /* 灰色 */
        }
        /* 空格键特定样式 */
        QPushButton#SpaceKey {
             min-width: 200px; /* 更宽 */
        }
        /* 滑块凹槽样式 */
        QSlider::groove:horizontal {
            border: 1px solid #bbb;
            background: rgba(255, 255, 255, 150); /* 半透明白色 */
            height: 5px; /* 高度 */
            border-radius: 3px; /* 圆角 */
        }
        /* 滑块手柄样式 */
        QSlider::handle:horizontal {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #eee, stop:1 #ccc); /* 浅灰渐变 */
            border: 1px solid #777; /* 边框 */
            width: 16px; /* 宽度 */
            margin: -6px 0; /* 垂直居中 */
            border-radius: 8px; /* 圆角 */
        }
    )") // 使用 .arg() 替换占位符
                          .arg(QString::number(int(DEFAULT_OPACITY_PERCENT / 100.0 * 255))) // %1: 初始背景 alpha 值
                          .arg(KEY_MIN_HEIGHT - 10) // %2: 按钮最小高度 (QSS)
                          .arg(KEY_MIN_WIDTH - 10)  // %3: 按钮最小宽度 (QSS)
    );

    // --- 加载键盘布局数据 ---
    qRegisterMetaType<KeyInfo>("KeyInfo"); // 注册 KeyInfo 类型，用于 QVariant
    fullLayoutData = getFullKeyboardLayout(); // 获取完整布局
    splitLayout(fullLayoutData, leftLayoutData, rightLayoutData); // 拆分布局

    // --- 初始化键盘状态 ---
#ifdef _WIN32
    // 从操作系统获取 Caps Lock, Num Lock, Scroll Lock 的初始状态
    capsLockActive = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
    numLockActive = (GetKeyState(VK_NUMLOCK) & 0x0001) != 0;
    scrollLockActive = (GetKeyState(VK_SCROLL) & 0x0001) != 0;
    // 假设其他修饰键初始状态为未按下
    shiftActive = false; ctrlActive = false; altActive = false; winActive = false;
#else
    // 非 Windows 下假设初始状态都为 false
    capsLockActive = false; numLockActive = false; scrollLockActive = false;
    shiftActive = false; ctrlActive = false; altActive = false; winActive = false;
#endif

    // --- 设置 UI ---
    setupUI(); // 创建界面元素
    updateModifierKeysVisuals(); // 根据初始状态更新按键视觉效果
    // 使用 invokeMethod 确保在事件循环开始后再定位窗口，避免初始尺寸问题
    QMetaObject::invokeMethod(this, &VirtualKeyboardWidget::positionWindow, Qt::QueuedConnection);

    // --- 连接屏幕变化信号 ---
    // 当屏幕几何信息 (分辨率、任务栏位置等) 改变时，重新定位窗口
    if(QGuiApplication::primaryScreen()) {
        connect(QGuiApplication::primaryScreen(), &QScreen::geometryChanged, this, &VirtualKeyboardWidget::positionWindow);
    }
}

// --- setupUI: 初始化用户界面 ---
void VirtualKeyboardWidget::setupUI() {
    // 创建最外层垂直布局
    outerLayout = new QVBoxLayout(this); // 'this' 作为父对象，布局将设置给主窗口
    outerLayout->setContentsMargins(5, 5, 5, 5); // 设置外边距
    outerLayout->setSpacing(5); // 设置元素间距

    // 创建容纳左右键盘的水平布局
    keyboardLayout = new QHBoxLayout();
    keyboardLayout->setContentsMargins(0, 0, 0, 0); // 无内边距
    keyboardLayout->setSpacing(10); // 左右键盘间距

    // --- 创建左键盘 ---
    leftKeyboardWidget = new QWidget(); // 创建左侧容器
    leftKeyboardWidget->setObjectName("KeyboardHalf"); // 设置对象名，用于 QSS 选择器
    leftKeyboardWidget->setAttribute(Qt::WA_StyledBackground); // 允许 QSS 设置背景
    leftKeyboardWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum); // 设置尺寸策略
    leftGridLayout = new QGridLayout(leftKeyboardWidget); // 创建网格布局并设置给左侧容器
    leftGridLayout->setSpacing(4); // 按键间距
    createKeyboardLayout(leftKeyboardWidget, leftGridLayout, leftLayoutData); // 生成左侧按键
    keyboardLayout->addWidget(leftKeyboardWidget, 1); // 添加到水平布局，权重为 1

    // --- 添加伸缩项 ---
    keyboardLayout->addStretch(1); // 在左右键盘中间添加可伸缩空间

    // --- 创建右键盘 ---
    rightKeyboardWidget = new QWidget(); // 创建右侧容器
    rightKeyboardWidget->setObjectName("KeyboardHalf");
    rightKeyboardWidget->setAttribute(Qt::WA_StyledBackground);
    rightKeyboardWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    rightGridLayout = new QGridLayout(rightKeyboardWidget); // 创建网格布局
    rightGridLayout->setSpacing(4);
    createKeyboardLayout(rightKeyboardWidget, rightGridLayout, rightLayoutData); // 生成右侧按键
    keyboardLayout->addWidget(rightKeyboardWidget, 1); // 添加到水平布局，权重为 1

    // 将包含左右键盘的水平布局添加到外层垂直布局
    outerLayout->addLayout(keyboardLayout);

    // --- 创建透明度滑块 ---
    opacitySlider = new QSlider(Qt::Horizontal); // 创建水平滑块
    opacitySlider->setRange(static_cast<int>(MIN_OPACITY * 100), static_cast<int>(MAX_OPACITY * 100)); // 设置范围 (20-100)
    opacitySlider->setValue(DEFAULT_OPACITY_PERCENT); // 设置初始值
    opacitySlider->setFixedHeight(20); // 设置固定高度
    opacitySlider->setToolTip("调节键盘透明度"); // 设置鼠标悬停提示
    // 注意: 由于主窗口设置了 Qt::WindowDoesNotAcceptFocus, 可能无法直接用鼠标操作滑块
    // 如果滑块无法操作，需要考虑其他方式调节透明度 (如右键菜单)
    // opacitySlider->setFocusPolicy(Qt::StrongFocus); // 尝试设置焦点策略，但可能无效
    connect(opacitySlider, &QSlider::valueChanged, this, &VirtualKeyboardWidget::changeOpacity); // 连接信号槽
    outerLayout->addWidget(opacitySlider); // 将滑块添加到外层布局底部
}

// --- createKeyboardLayout: 根据布局数据创建按钮 ---
void VirtualKeyboardWidget::createKeyboardLayout(QWidget* parentWidget, QGridLayout* layout, const KeyboardLayout& keyRows)
{
    // 遍历布局数据中的每一行
    for (const auto& row : keyRows) {
        // 遍历当前行的每一个按键信息
        for (const auto& keyInfo : row) {
            // 跳过完全空的占位符
            if (keyInfo.vkCode == 0 && keyInfo.text.isEmpty()) continue;

            // 创建按钮
            QPushButton *button = new QPushButton(keyInfo.text, parentWidget);
            // 设置尺寸策略为可扩展
            button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            // !!! 关键: 设置按钮不接受焦点 !!!
            button->setFocusPolicy(Qt::NoFocus);

            // 根据按键类型或 VK 码设置对象名，用于 QSS 样式区分
            if (keyInfo.vkCode == VK_SPACE) button->setObjectName("SpaceKey");
            else if (keyInfo.type != KeyType::Normal) button->setObjectName("SpecialKey"); // 非普通键使用 SpecialKey 样式

            // 使切换键 (Caps Lock 等) 可被选中 (checkable)
            if (keyInfo.type == KeyType::ModifierToggle) {
                button->setCheckable(true);
            }

            // --- 设置按键自动重复 ---
            bool enableAutoRepeat = false;
            switch (keyInfo.type) {
                case KeyType::Normal: // 普通字符键允许重复
                    enableAutoRepeat = true;
                    break;
                case KeyType::Special: // 特殊键只允许部分重复 (如 Backspace, Delete, Space, 方向键)
                    if (keyInfo.vkCode == VK_BACK || keyInfo.vkCode == VK_DELETE || keyInfo.vkCode == VK_SPACE ||
                        keyInfo.vkCode == VK_LEFT || keyInfo.vkCode == VK_RIGHT || keyInfo.vkCode == VK_UP || keyInfo.vkCode == VK_DOWN) {
                        enableAutoRepeat = true;
                    }
                    break;
                default: // 修饰键和切换键不自动重复
                    enableAutoRepeat = false;
                    break;
            }
            if (enableAutoRepeat) {
                button->setAutoRepeat(true); // 允许自动重复
                button->setAutoRepeatDelay(AUTO_REPEAT_DELAY_MS); // 设置重复延迟
                // button->setAutoRepeatInterval(100); // 可选：设置重复间隔
            }
            // --- 结束自动重复设置 ---

            // 将按键信息 (KeyInfo) 存储为按钮的属性，方便后续在槽函数中获取
            button->setProperty("keyInfo", QVariant::fromValue(keyInfo));

            // 连接按钮的 pressed 和 released 信号到对应的槽函数
            // 使用 pressed/released 可以更好地处理按键按下和抬起事件，特别是对于修饰键
            connect(button, &QPushButton::pressed, this, &VirtualKeyboardWidget::onKeyPressed);
            connect(button, &QPushButton::released, this, &VirtualKeyboardWidget::onKeyReleased);

            // 将按钮添加到网格布局中，指定行、列、行跨度(1)、列跨度(keyInfo.columnSpan)
            layout->addWidget(button, keyInfo.row, keyInfo.column, 1, keyInfo.columnSpan);
            // 将按钮指针添加到列表中，方便后续统一更新视觉状态
            keyButtons.append(button);
        }
    }
    // 设置布局的行和列拉伸因子，使按钮在调整大小时能均匀填充空间
    for(int r=0; r < layout->rowCount(); ++r) layout->setRowStretch(r, 1);
    for(int c=0; c < layout->columnCount(); ++c) layout->setColumnStretch(c, 1);
}

// --- onKeyPressed: 处理按钮按下事件 ---
// 使用 SendInput 实现，采用“按下保持”的修饰键逻辑
void VirtualKeyboardWidget::onKeyPressed() {
    // 获取发送信号的按钮
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return; // 如果转换失败，则返回

    // 从按钮属性中获取 KeyInfo
    QVariant variant = button->property("keyInfo");
    if (!variant.isValid() || !variant.canConvert<KeyInfo>()) return; // 检查 QVariant 是否有效且可转换
    KeyInfo keyInfo = variant.value<KeyInfo>(); // 获取 KeyInfo 对象
    if (keyInfo.vkCode == 0) return; // 如果没有 VK Code，则忽略

    // 调试输出：按下的键和当前键盘窗口是否是活动窗口 (应为 false)
    qDebug() << "按下:" << keyInfo.text << "VK Code:" << keyInfo.vkCode << "| 键盘窗口活动:" << this->isActiveWindow();

    // 根据按键类型处理
    switch (keyInfo.type) {
        case KeyType::ModifierSticky: // 处理 Shift, Ctrl, Alt, Win 键
        { // 增加作用域以允许变量定义
            // 检查该修饰键是否已处于按下状态
            bool stateAlreadyActive = false;
            if ((keyInfo.vkCode == VK_LSHIFT || keyInfo.vkCode == VK_RSHIFT) && shiftActive) stateAlreadyActive = true;
            else if ((keyInfo.vkCode == VK_LCONTROL || keyInfo.vkCode == VK_RCONTROL) && ctrlActive) stateAlreadyActive = true;
            else if ((keyInfo.vkCode == VK_LMENU || keyInfo.vkCode == VK_RMENU) && altActive) stateAlreadyActive = true;
            else if ((keyInfo.vkCode == VK_LWIN || keyInfo.vkCode == VK_RWIN) && winActive) stateAlreadyActive = true;

            // 如果尚未按下，则更新内部状态，模拟按键按下，并更新视觉效果
            if (!stateAlreadyActive) {
                if (keyInfo.vkCode == VK_LSHIFT || keyInfo.vkCode == VK_RSHIFT) shiftActive = true;
                else if (keyInfo.vkCode == VK_LCONTROL || keyInfo.vkCode == VK_RCONTROL) ctrlActive = true;
                else if (keyInfo.vkCode == VK_LMENU || keyInfo.vkCode == VK_RMENU) altActive = true;
                else if (keyInfo.vkCode == VK_LWIN || keyInfo.vkCode == VK_RWIN) winActive = true;

                // 模拟按键按下事件
                simulateKey(keyInfo.vkCode, keyInfo.scanCode, true, keyInfo.isExtendedKey);
                // 更新所有按键的视觉状态
                updateModifierKeysVisuals();
            }
            break; // 结束 ModifierSticky case
        } // 结束作用域
        case KeyType::ModifierToggle: // 处理 Caps Lock, Num Lock, Scroll Lock
        { // 增加作用域
            // 切换内部状态
            if (keyInfo.vkCode == VK_CAPITAL) capsLockActive = !capsLockActive;
            else if (keyInfo.vkCode == VK_NUMLOCK) numLockActive = !numLockActive;
            else if (keyInfo.vkCode == VK_SCROLL) scrollLockActive = !scrollLockActive;

            // 模拟一次完整的按下和释放事件，以触发操作系统的状态切换
            simulateKey(keyInfo.vkCode, keyInfo.scanCode, true, keyInfo.isExtendedKey);
            simulateKey(keyInfo.vkCode, keyInfo.scanCode, false, keyInfo.isExtendedKey);

            // 根据新的内部状态更新视觉效果
            updateModifierKeysVisuals();
            break; // 结束 ModifierToggle case
        } // 结束作用域
        case KeyType::Normal: // 处理普通字符键
        case KeyType::Special: // 处理特殊功能键 (如 Enter, Backspace)
        { // 增加作用域
            // 只模拟按键按下事件 (释放事件在 onKeyReleased 中处理)
            simulateKey(keyInfo.vkCode, keyInfo.scanCode, true, keyInfo.isExtendedKey);
            // 注意：在此“按下保持”模型中，按下普通键不会自动释放修饰键
            break; // 结束 Normal/Special case
        } // 结束作用域
    } // 结束 switch
}

// --- onKeyReleased: 处理按钮释放事件 ---
// 使用 SendInput 实现，采用“按下保持”的修饰键逻辑
void VirtualKeyboardWidget::onKeyReleased() {
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    QVariant variant = button->property("keyInfo");
    if (!variant.isValid() || !variant.canConvert<KeyInfo>()) return;
    KeyInfo keyInfo = variant.value<KeyInfo>();
    if (keyInfo.vkCode == 0) return;

    qDebug() << "释放:" << keyInfo.text;

    switch (keyInfo.type) {
        case KeyType::ModifierSticky: // 处理 Shift, Ctrl, Alt, Win 键释放
            // 更新内部状态为未按下
            if (keyInfo.vkCode == VK_LSHIFT || keyInfo.vkCode == VK_RSHIFT) shiftActive = false;
            else if (keyInfo.vkCode == VK_LCONTROL || keyInfo.vkCode == VK_RCONTROL) ctrlActive = false;
            else if (keyInfo.vkCode == VK_LMENU || keyInfo.vkCode == VK_RMENU) altActive = false;
            else if (keyInfo.vkCode == VK_LWIN || keyInfo.vkCode == VK_RWIN) winActive = false;

            // 模拟按键释放事件
            simulateKey(keyInfo.vkCode, keyInfo.scanCode, false, keyInfo.isExtendedKey);
            // 更新视觉效果
            updateModifierKeysVisuals();
            break;
        case KeyType::ModifierToggle: // Caps Lock 等切换键在释放时无操作
            break;
        case KeyType::Normal: // 普通字符键释放
        case KeyType::Special: // 特殊功能键释放
            // 模拟按键释放事件
            simulateKey(keyInfo.vkCode, keyInfo.scanCode, false, keyInfo.isExtendedKey);
            break;
    }
}


// --- updateModifierKeysVisuals: 更新所有按键的视觉状态 ---
void VirtualKeyboardWidget::updateModifierKeysVisuals() {
    // 计算有效的 Shift 状态 (考虑 Caps Lock 对字母的影响)
    bool effectiveShift = shiftActive ^ capsLockActive;

    // 遍历所有按键按钮
    for (QPushButton* button : keyButtons) {
        QVariant variant = button->property("keyInfo");
        if (!variant.isValid() || !variant.canConvert<KeyInfo>()) continue;
        KeyInfo keyInfo = variant.value<KeyInfo>();

        // --- 更新按钮文本 (大小写/符号切换) ---
        if (keyInfo.type == KeyType::Normal) { // 只处理普通键
            bool isLetter = (keyInfo.text.length() == 1 && keyInfo.text.at(0).isLetter());
            if (!keyInfo.shiftedText.isEmpty()) { // 如果有 Shift 状态的文本
                if (isLetter) {
                    // 字母的大小写取决于 effectiveShift (Shift XOR CapsLock)
                    button->setText(effectiveShift ? keyInfo.shiftedText : keyInfo.text);
                } else {
                    // 数字和符号只取决于 shiftActive
                    button->setText(shiftActive ? keyInfo.shiftedText : keyInfo.text);
                }
            }
            // 如果没有 shiftedText，则文本保持不变
        }

        // --- 更新按钮视觉样式和选中状态 ---
        bool isActive = false; // 标记当前按键是否处于“激活”状态 (用于样式)
        QString styleObjectName = "SpecialKey"; // 默认对象名，用于 QSS

        if (keyInfo.type == KeyType::ModifierSticky) { // Shift, Ctrl, Alt, Win
            // 根据内部状态判断是否激活
            if ((keyInfo.vkCode == VK_LSHIFT || keyInfo.vkCode == VK_RSHIFT) && shiftActive) isActive = true;
            else if ((keyInfo.vkCode == VK_LCONTROL || keyInfo.vkCode == VK_RCONTROL) && ctrlActive) isActive = true;
            else if ((keyInfo.vkCode == VK_LMENU || keyInfo.vkCode == VK_RMENU) && altActive) isActive = true;
            else if ((keyInfo.vkCode == VK_LWIN || keyInfo.vkCode == VK_RWIN) && winActive) isActive = true;
            // 根据激活状态设置 QSS 对象名
            styleObjectName = isActive ? "ModifierActive" : "SpecialKey";
            // 这些键本身不是 checkable 的
            button->setCheckable(false);
            button->setChecked(false); // 确保它们不处于视觉选中状态

        } else if (keyInfo.type == KeyType::ModifierToggle) { // Caps Lock 等
            // 根据内部状态判断是否激活
            if (keyInfo.vkCode == VK_CAPITAL) isActive = capsLockActive;
            else if (keyInfo.vkCode == VK_NUMLOCK) isActive = numLockActive;
            else if (keyInfo.vkCode == VK_SCROLL) isActive = scrollLockActive;
            // 设置按钮的选中状态 (因为它们是 checkable 的)
            button->setChecked(isActive);
            // 根据激活状态设置 QSS 对象名
            styleObjectName = isActive ? "ToggleActive" : "SpecialKey";

        } else if (keyInfo.vkCode == VK_SPACE) { // 空格键
            styleObjectName = "SpaceKey";
        } else if (keyInfo.type == KeyType::Normal) { // 普通键
            styleObjectName = ""; // 不需要特殊的对象名
        }
        // 对于其他 SpecialKey，保持默认的 "SpecialKey"

        // 如果对象名需要改变，则更新并重新应用样式
        if (button->objectName() != styleObjectName) {
            button->setObjectName(styleObjectName);
            style()->unpolish(button); // 移除旧样式
            style()->polish(button);   // 应用新样式 (基于新对象名)
        }
    }
}

// --- simulateKey: 使用 SendInput 模拟按键 ---
void VirtualKeyboardWidget::simulateKey(int vkCode, int scanCode, bool press, bool isExtended) {
#ifdef _WIN32 // 只在 Windows 下有效
    if (vkCode == 0) return; // 无效的 VK Code

    INPUT input = { 0 }; // 初始化 INPUT 结构体
    input.type = INPUT_KEYBOARD; // 指定为键盘输入

    // 决定是使用扫描码还是虚拟键码 (当前强制使用 VK Code，实践中可能更稳定)
    bool useScanCode = false; // 设置为 true 以尝试使用扫描码
    // bool useScanCode = (scanCode != 0); // 或者，如果扫描码有效则使用它

    if (useScanCode && scanCode != 0) {
        input.ki.wScan = static_cast<WORD>(scanCode); // 设置扫描码
        input.ki.dwFlags = KEYEVENTF_SCANCODE;       // 标志：使用扫描码
    } else {
        input.ki.wVk = static_cast<WORD>(vkCode);     // 设置虚拟键码
        input.ki.dwFlags = 0;                         // 标志：使用虚拟键码 (默认)
    }

    // 如果是释放事件，添加 KEYEVENTF_KEYUP 标志
    if (!press) {
        input.ki.dwFlags |= KEYEVENTF_KEYUP;
    }

    // 如果是扩展键，添加 KEYEVENTF_EXTENDEDKEY 标志
    if (isExtended) {
        input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }

    // 调用 SendInput 包装函数发送事件
    sendInputWrapper(input);
#else
    // 非 Windows 平台提示不支持
    Q_UNUSED(vkCode); Q_UNUSED(scanCode); Q_UNUSED(press); Q_UNUSED(isExtended); // 避免未使用变量警告
    qWarning("键盘模拟功能仅在 Windows 平台可用。");
#endif
}

// --- sendInputWrapper: SendInput API 的包装，带日志 ---
void VirtualKeyboardWidget::sendInputWrapper(INPUT input) {
#ifdef _WIN32 // 只在 Windows 下有效
    // --- 获取并记录当前前台窗口信息 ---
    HWND fgWin = GetForegroundWindow(); // 获取当前拥有焦点的窗口句柄
    QString windowTitle = "N/A";
    DWORD processId = 0; // 进程 ID
    if (fgWin) {
        wchar_t titleBuffer[256]; // 宽字符缓冲区用于窗口标题
        // 获取窗口标题
        if (GetWindowTextW(fgWin, titleBuffer, 256) > 0) {
            windowTitle = QString::fromWCharArray(titleBuffer);
        } else {
            windowTitle = "<无标题>";
        }
        // 获取窗口所属进程 ID
        GetWindowThreadProcessId(fgWin, &processId);
    } else {
        windowTitle = "<无前台窗口>";
    }
    // 打印日志：尝试向哪个窗口发送输入
    qDebug() << "  尝试发送 SendInput 到:" << windowTitle << "(PID:" << processId << ")";
    // --- 结束记录窗口信息 ---

    // 调用 SendInput 函数
    UINT result = SendInput(1, &input, sizeof(INPUT)); // 发送 1 个 INPUT 事件

    // 检查 SendInput 是否成功
    if (result != 1) {
        DWORD errorCode = GetLastError(); // 获取错误码
        qWarning() << "  SendInput 失败! Result:" << result << " 错误码:" << errorCode;
        // 常见的错误码:
        // 5: ERROR_ACCESS_DENIED (访问被拒绝 - 通常是因为 UIPI 权限问题，目标窗口权限更高)
        // 87: ERROR_INVALID_PARAMETER (无效参数 - 检查 INPUT 结构体或标志)
        // 1400: ERROR_INVALID_WINDOW_HANDLE (无效窗口句柄 - 不太可能在这里发生)
    } else {
        // qDebug() << "  SendInput OK."; // 可选：成功时打印日志
    }
#else
    Q_UNUSED(input); // 避免未使用变量警告
#endif
}

// --- changeOpacity: 响应滑块改变，更新透明度 ---
void VirtualKeyboardWidget::changeOpacity(int value) {
    // 将滑块值 (百分比) 转换为 0.0 到 1.0 的浮点数
    double opacity = static_cast<double>(value) / 100.0;
    // 将浮点透明度转换为 0-255 的 alpha 值
    int alpha = qBound(0, static_cast<int>(opacity * 255), 255);

    // 通过正则表达式更新 QSS 中 rgba 背景色的 alpha 值
    QString currentStyle = styleSheet(); // 获取当前样式表
    // 正则表达式查找 "rgba(数字,数字,数字, 数字)" 模式
    // \\d+ 匹配一个或多个数字, \\s* 匹配零个或多个空白符
    // (\\d+,\\s*\\d+,\\s*\\d+,\\s*) 捕获 RGB 部分 (捕获组 1)
    // \\d+ 匹配旧的 alpha 值
    currentStyle.replace(QRegularExpression("rgba\\((\\d+,\\s*\\d+,\\s*\\d+,\\s*)\\d+\\)"),
                         QString("rgba(\\1%1)").arg(alpha)); // 替换为新的 alpha 值，保留 RGB 部分 (\1)
    setStyleSheet(currentStyle); // 应用修改后的样式表
}

// --- positionWindow: 定位窗口到屏幕底部任务栏上方 ---
void VirtualKeyboardWidget::positionWindow() {
    QScreen *screen = QGuiApplication::primaryScreen(); // 获取主屏幕
    if (!screen) return; // 如果获取失败则返回

    // 获取屏幕的可用几何区域 (通常不包括任务栏)
    QRect availableGeometry = screen->availableGeometry();

    // 计算键盘窗口期望的高度 (基于布局的建议高度，并设置一个最小值)
    int desiredHeight = outerLayout->sizeHint().height(); // 获取外层布局的建议高度
    // 确保高度不小于一个基于按键高度估算的最小值 (例如 5 行按键的高度)
    desiredHeight = qMax(desiredHeight, (KEY_MIN_HEIGHT + 10) * 5);

    // 计算新的 Y 坐标：屏幕可用区域底部 - 窗口高度 + 1 (通常刚好在任务栏上面)
    int newY = availableGeometry.bottom() - desiredHeight + 1;
    // 新的 X 坐标：屏幕可用区域左边界
    int newX = availableGeometry.left();
    // 新的宽度：屏幕可用区域宽度
    int newWidth = availableGeometry.width();

    // 调试输出窗口定位信息
    qDebug() << "定位窗口: 屏幕可用区域 =" << availableGeometry << "期望高度 =" << desiredHeight;
    qDebug() << "设置几何区域为:" << QRect(newX, newY, newWidth, desiredHeight);

    // 移动并调整窗口大小
    // 使用 move 和 resize 分开调用有时比 setGeometry 更能避免 resizeEvent 的递归问题
    move(newX, newY);
    resize(newWidth, desiredHeight);
    // setGeometry(newX, newY, newWidth, desiredHeight); // 也可以使用这个
}

// --- resizeEvent: 处理窗口尺寸改变事件 ---
void VirtualKeyboardWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event); // 调用基类的处理函数
    // 可选：如果希望窗口在手动调整大小后仍然强制停靠底部，可以在这里重新调用 positionWindow()
    // 但需要注意避免无限递归调用 (例如，通过标志或定时器延迟调用)
    // QMetaObject::invokeMethod(this, "positionWindow", Qt::QueuedConnection);
    qDebug() << "窗口尺寸调整为:" << event->size(); // 调试输出新的尺寸
}
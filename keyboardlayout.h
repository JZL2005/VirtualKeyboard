#ifndef VIRTUALKEYBOARD_KEYBOARDLAYOUT_H
#define VIRTUALKEYBOARD_KEYBOARDLAYOUT_H

#include <QString>
#include <QList>
#include <QVariant> // QVariant::fromValue 需要
#include <QDebug>   // qDebug 需要

// --- Windows API 头文件 ---
#ifdef _WIN32
// WIN32_LEAN_AND_MEAN 和 NOMINMAX 宏由 CMakeLists.txt 定义
#include <windows.h>
#include <winuser.h> // 包含 VK_ 常量和 MapVirtualKey 等定义
#else
// 如果在非 Windows 平台编译，定义占位符 (虽然 SendInput 功能将不可用)
#define VK_SHIFT       0x10
#define VK_LSHIFT      0xA0
#define VK_RSHIFT      0xA1
#define VK_CONTROL     0x11
#define VK_LCONTROL    0xA2
#define VK_RCONTROL    0xA3
#define VK_MENU        0x12 // Alt
#define VK_LMENU       0xA4
#define VK_RMENU       0xA5
#define VK_LWIN        0x5B
#define VK_RWIN        0x5C
#define VK_APPS        0x5D // 上下文菜单键 (应用程序键)
#define VK_CAPITAL     0x14 // Caps Lock
#define VK_NUMLOCK     0x90 // Num Lock
#define VK_SCROLL      0x91 // Scroll Lock
#define VK_BACK        0x08 // Backspace
#define VK_TAB         0x09
#define VK_RETURN      0x0D // Enter
#define VK_ESCAPE      0x1B
#define VK_SPACE       0x20
#define VK_PRIOR       0x21 // Page Up
#define VK_NEXT        0x22 // Page Down
#define VK_END         0x23
#define VK_HOME        0x24
#define VK_LEFT        0x25 // 方向键 左
#define VK_UP          0x26 // 方向键 上
#define VK_RIGHT       0x27 // 方向键 右
#define VK_DOWN        0x28 // 方向键 下
#define VK_INSERT      0x2D
#define VK_DELETE      0x2E
#define VK_SNAPSHOT    0x2C // Print Screen
#define VK_PAUSE       0x13 // Pause/Break
#define VK_F1          0x70
#define VK_F2          0x71
#define VK_F3          0x72
#define VK_F4          0x73
#define VK_F5          0x74
#define VK_F6          0x75
#define VK_F7          0x76
#define VK_F8          0x77
#define VK_F9          0x78
#define VK_F10         0x79
#define VK_F11         0x7A
#define VK_F12         0x7B
// OEM 键码，用于标点符号等，具体映射可能因键盘布局而异
#define VK_OEM_3       0xC0 // `~
#define VK_OEM_MINUS   0xBD // -_
#define VK_OEM_PLUS    0xBB // =+
#define VK_OEM_4       0xDB // [{
#define VK_OEM_6       0xDD // ]}
#define VK_OEM_5       0xDC // \|
#define VK_OEM_1       0xBA // ;:
#define VK_OEM_7       0xDE // '"
#define VK_OEM_COMMA   0xBC // ,<
#define VK_OEM_PERIOD  0xBE // .>
#define VK_OEM_2       0xBF // /?
#endif // _WIN32

// 按键类型枚举
enum class KeyType {
    Normal,         // 普通可打印字符键 (a-z, 0-9, 符号等)
    ModifierSticky, // 修饰键 (Shift, Ctrl, Alt, Win) - 此处实现为“按下保持”行为
    ModifierToggle, // 切换键 (Caps Lock, Num Lock, Scroll Lock) - 按下切换状态
    Special         // 特殊功能键 (Enter, Backspace, 方向键, F1-F12, Esc 等)
};

// 存储按键信息的结构体 (用于 SendInput 版本)
struct KeyInfo {
    QString text;         // 按键上显示的默认文本
    QString shiftedText;  // 按下 Shift 键时应显示的文本
    int vkCode;           // Windows 虚拟键码 (Virtual Key Code)
    int scanCode = 0;     // 硬件扫描码 (Scan Code) - 可选，但 SendInput 可能更需要
    KeyType type = KeyType::Normal; // 按键类型
    int row = 0;          // 在网格布局中的行号 (如果为0，则自动分配)
    int column = 0;       // 在网格布局中的列号 (如果为0，则自动分配)
    int columnSpan = 1;   // 按键跨越的列数
    bool isExtendedKey = false; // 是否为扩展键 (对于 SendInput 很重要，如右 Ctrl/Alt, 方向键等)

    KeyInfo() = default;  // QVariant 需要默认构造函数
    // 构造函数，包含 scanCode 和 isExtendedKey
    KeyInfo(QString t, QString st, int vk, int sc = 0, KeyType kt = KeyType::Normal, int r = 0, int c = 0, int cs = 1, bool ext = false)
            : text(t), shiftedText(st), vkCode(vk), scanCode(sc), type(kt), row(r), column(c), columnSpan(cs), isExtendedKey(ext) {}
};

// 注册 KeyInfo 结构体，以便能被 QVariant 使用 (用于按钮的 setProperty)
Q_DECLARE_METATYPE(KeyInfo);

// 定义键盘布局类型为一个二维列表，存储 KeyInfo
using KeyboardLayout = QList<QList<KeyInfo>>;

// --- getFullKeyboardLayout 函数 (SendInput 版本) ---
// 定义完整的键盘布局数据
inline KeyboardLayout getFullKeyboardLayout() {
    // 布局定义，使用 VK_* 常量，并为需要扩展标志的键设置 isExtendedKey = true
    KeyboardLayout layout = {
            // 第 0 行: 功能键等
            { {"Esc", "", VK_ESCAPE}, {"F1", "", VK_F1}, {"F2", "", VK_F2}, {"F3", "", VK_F3}, {"F4", "", VK_F4}, {"F5", "", VK_F5}, {"F6", "", VK_F6}, {"F7", "", VK_F7}, {"F8", "", VK_F8}, {"F9", "", VK_F9}, {"F10", "", VK_F10}, {"F11", "", VK_F11}, {"F12", "", VK_F12},
                                                                                                                                                                                                                                                                                       {"PrtSc", "", VK_SNAPSHOT, 0, KeyType::Special, 0, 13, 1, true}, // PrtSc 是扩展键
                    {"ScrLk", "", VK_SCROLL, 0, KeyType::ModifierToggle, 0, 14, 1, false}, // ScrLk 通常不是扩展键
                    {"Pause", "", VK_PAUSE, 0, KeyType::Special, 0, 15, 1, false} },    // Pause 通常不是扩展键 (除非 Ctrl+Pause)
            // 第 1 行: 数字和符号
            { {"`", "~", VK_OEM_3}, {"1", "!", '1'}, {"2", "@", '2'}, {"3", "#", '3'}, {"4", "$", '4'}, {"5", "%", '5'}, {"6", "^", '6'}, {"7", "&", '7'}, {"8", "*", '8'}, {"9", "(", '9'}, {"0", ")", '0'}, {"-", "_", VK_OEM_MINUS}, {"=", "+", VK_OEM_PLUS}, {"Backspace", "", VK_BACK, 0, KeyType::Special, 1, 13, 3, false} }, // Backspace 通常不是扩展键
            // 第 2 行: QWERTY 行
            { {"Tab", "", VK_TAB, 0, KeyType::Special, 2, 0, 2}, {"Q", "Q", 'Q'}, {"W", "W", 'W'}, {"E", "E", 'E'}, {"R", "R", 'R'}, {"T", "T", 'T'}, {"Y", "Y", 'Y'}, {"U", "U", 'U'}, {"I", "I", 'I'}, {"O", "O", 'O'}, {"P", "P", 'P'}, {"[", "{", VK_OEM_4}, {"]", "}", VK_OEM_6}, {"\\", "|", VK_OEM_5, 0, KeyType::Normal, 2, 13, 2} },
            // 第 3 行: ASDF 行
            { {"Caps", "", VK_CAPITAL, 0, KeyType::ModifierToggle, 3, 0, 2}, {"A", "A", 'A'}, {"S", "S", 'S'}, {"D", "D", 'D'}, {"F", "F", 'F'}, {"G", "G", 'G'}, {"H", "H", 'H'}, {"J", "J", 'J'}, {"K", "K", 'K'}, {"L", "L", 'L'}, {";", ":", VK_OEM_1}, {"'", "\"", VK_OEM_7}, {"Enter", "", VK_RETURN, 0, KeyType::Special, 3, 12, 3, false} }, // 主 Enter 通常不是扩展键 (小键盘 Enter 是)
            // 第 4 行: ZXCV 行
            { {"Shift", "", VK_LSHIFT, 0, KeyType::ModifierSticky, 4, 0, 3}, {"Z", "Z", 'Z'}, {"X", "X", 'X'}, {"C", "C", 'C'}, {"V", "V", 'V'}, {"B", "B", 'B'}, {"N", "N", 'N'}, {"M", "M", 'M'}, {",", "<", VK_OEM_COMMA}, {".", ">", VK_OEM_PERIOD}, {"/", "?", VK_OEM_2}, {"Shift", "", VK_RSHIFT, 0, KeyType::ModifierSticky, 4, 11, 3, false} }, // 右 Shift 通常不是扩展键
            // 第 5 行: 底部行 (注意 RCtrl, RAlt, RWin, Apps 是扩展键)
            { {"Ctrl", "", VK_LCONTROL, 0, KeyType::ModifierSticky, 5, 0, 2, false}, {"Win", "", VK_LWIN, 0, KeyType::ModifierSticky, 5, 2, 1, true}, {"Alt", "", VK_LMENU, 0, KeyType::ModifierSticky, 5, 3, 1, false}, {"Space", "", VK_SPACE, 0, KeyType::Special, 5, 4, 7, false}, {"Alt", "", VK_RMENU, 0, KeyType::ModifierSticky, 5, 11, 1, true}, {"Win", "", VK_RWIN, 0, KeyType::ModifierSticky, 5, 12, 1, true}, {"Menu", "", VK_APPS, 0, KeyType::Special, 5, 13, 1, true}, {"Ctrl", "", VK_RCONTROL, 0, KeyType::ModifierSticky, 5, 14, 2, true} },
            // 第 6 行: 方向键 & 导航键 (通常是扩展键)
            { {"Ins", "", VK_INSERT, 0, KeyType::Special, 6, 0, 1, true}, {"Del", "", VK_DELETE, 0, KeyType::Special, 6, 1, 1, true}, {"Home", "", VK_HOME, 0, KeyType::Special, 6, 2, 1, true}, {"End", "", VK_END, 0, KeyType::Special, 6, 3, 1, true}, {"PgUp", "", VK_PRIOR, 0, KeyType::Special, 6, 4, 1, true}, {"PgDn", "", VK_NEXT, 0, KeyType::Special, 6, 5, 1, true},
                    {"", "", 0, 0, KeyType::Special, 6, 6, 3}, // 占位符，用于布局
                    {"↑", "", VK_UP,    0, KeyType::Special, 6, 9, 1, true}, {"", "", 0, 0, KeyType::Special, 6, 10, 1}, // 占位符
                    {"←", "", VK_LEFT,  0, KeyType::Special, 6, 11, 1, true}, {"↓", "", VK_DOWN,  0, KeyType::Special, 6, 12, 1, true}, {"→", "", VK_RIGHT, 0, KeyType::Special, 6, 13, 1, true} }
    };

    // 自动分配行号、列号，并尝试获取扫描码
    int max_cols = 0;
    for (int r = 0; r < layout.size(); ++r) {
        int current_col = 0;
        for (int c = 0; c < layout[r].size(); ++c) {
            // 如果行号或列号是默认值0，则自动分配
            if (layout[r][c].row == 0 && layout[r][c].column == 0 && !(r==0 && c==0)) {
                layout[r][c].row = r;
                layout[r][c].column = current_col;
                // 如果行号正确，但列号需要根据前面的跨度调整
            } else if (layout[r][c].row == r && layout[r][c].column >= current_col) {
                layout[r][c].column = current_col;
            }
            current_col += layout[r][c].columnSpan; // 累加列跨度

            // 如果扫描码未设置且有 VK 码，尝试从 VK 码获取扫描码
            // 注意: MapVirtualKey(..., MAPVK_VK_TO_VSC) 对某些键可能不准确
            if (layout[r][c].scanCode == 0 && layout[r][c].vkCode != 0) {
#ifdef _WIN32
                layout[r][c].scanCode = MapVirtualKey(layout[r][c].vkCode, MAPVK_VK_TO_VSC);
#endif
            }
        }
        if (current_col > max_cols) max_cols = current_col; // 记录最大列数
    }
    return layout;
}

// --- splitLayout 函数 (拆分键盘布局) ---
// 根据定义的拆分点将完整布局拆分为左右两部分
inline void splitLayout(const KeyboardLayout& fullLayout, KeyboardLayout& leftLayout, KeyboardLayout& rightLayout) {
    leftLayout.clear(); rightLayout.clear(); // 清空目标布局

    // 定义属于左半部分的按键 VK 码列表 (基于 TGB 拆分)
    const QList<int> leftSplitVKs = { VK_ESCAPE, VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, // F键行
                                      VK_OEM_3, '1', '2', '3', '4', '5',             // 数字行
                                      VK_TAB, 'Q', 'W', 'E', 'R', 'T',              // QWERTY 行
                                      VK_CAPITAL, 'A', 'S', 'D', 'F', 'G',          // ASDF 行
                                      VK_LSHIFT, 'Z', 'X', 'C', 'V', 'B',           // ZXCV 行
                                      VK_LCONTROL, VK_LWIN, VK_LMENU};             // 底部行左侧

    // 定义属于右半部分的按键 VK 码列表 (基于 YHN 拆分)
    const QList<int> rightSplitVKs = { VK_F6, VK_F7, VK_F8, VK_F9, VK_F10, VK_F11, VK_F12, VK_SNAPSHOT, VK_SCROLL, VK_PAUSE, // F键行
                                       '6', '7', '8', '9', '0', VK_OEM_MINUS, VK_OEM_PLUS, VK_BACK, // 数字行
                                       'Y', 'U', 'I', 'O', 'P', VK_OEM_4, VK_OEM_6, VK_OEM_5,      // QWERTY 行
                                       'H', 'J', 'K', 'L', VK_OEM_1, VK_OEM_7, VK_RETURN,           // ASDF 行
                                       'N', 'M', VK_OEM_COMMA, VK_OEM_PERIOD, VK_OEM_2, VK_RSHIFT, // ZXCV 行
                                       VK_RMENU, VK_RWIN, VK_APPS, VK_RCONTROL,                   // 底部行右侧
            // 方向键和导航键通常放右侧
                                       VK_INSERT, VK_DELETE, VK_HOME, VK_END, VK_PRIOR, VK_NEXT,
                                       VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT };

    // 遍历完整布局的每一行
    for (const auto& row : fullLayout) {
        QList<KeyInfo> leftRow, rightRow; // 当前行的左右部分
        // 遍历当前行的每个按键
        for (const auto& key : row) {
            // 处理空占位符 (根据大致列位置分配)
            if (key.vkCode == 0 && !key.text.isEmpty()) {
                if (key.column < 8) leftRow.append(key); else rightRow.append(key);
                // 特殊处理空格键，将其拆分为两半
            } else if (key.vkCode == VK_SPACE) {
                KeyInfo leftSpace = key; leftSpace.columnSpan /= 2; leftSpace.text="Space"; leftRow.append(leftSpace);
                KeyInfo rightSpace = key; rightSpace.column = key.column + leftSpace.columnSpan; rightSpace.columnSpan -= leftSpace.columnSpan; rightSpace.text="Space"; rightRow.append(rightSpace);
                // 如果按键 VK 码在左侧列表
            } else if (leftSplitVKs.contains(key.vkCode)) {
                leftRow.append(key);
                // 如果按键 VK 码在右侧列表
            } else if (rightSplitVKs.contains(key.vkCode)) {
                rightRow.append(key);
                // 对于未明确分配的键 (可能布局定义不完整)，根据列号大致分配
            } else {
                if (key.column < 8) leftRow.append(key); else rightRow.append(key);
                // 如果需要，可以打印警告
                qWarning() << "警告: 键 '" << key.text << "' (VK:" << key.vkCode << ") 在 splitLayout 中未明确分配左右.";
            }
        }

        // 重新计算左右两半内部的列号，使其从0开始连续
        int lCol=0; for(int i=0; i<leftRow.size(); ++i){ leftRow[i].column=lCol; lCol+=leftRow[i].columnSpan; }
        int rCol=0; for(int i=0; i<rightRow.size(); ++i){ rightRow[i].column=rCol; rCol+=rightRow[i].columnSpan; }

        // 将处理好的行添加到最终的左右布局中
        if (!leftRow.isEmpty()) leftLayout.append(leftRow);
        if (!rightRow.isEmpty()) rightLayout.append(rightRow);
    }
}

#endif //VIRTUALKEYBOARD_KEYBOARDLAYOUT_H
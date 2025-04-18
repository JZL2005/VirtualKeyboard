# CMake 最低版本要求
cmake_minimum_required(VERSION 3.25)
# 项目名称、版本和语言
project(VirtualKeyboard VERSION 1.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用 Qt 的 CMake 特性，自动处理 MOC, RCC, UIC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt 6 必需的模块
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core)

# 定义可执行文件及其源文件
add_executable(VirtualKeyboard
        main.cpp
        virtualkeyboardwidget.h
        virtualkeyboardwidget.cpp
        keyboardlayout.h
        )

# 链接 Qt 库
target_link_libraries(VirtualKeyboard PRIVATE
        Qt6::Widgets
        Qt6::Gui
        Qt6::Core
        )

# 特定于平台的设置 (Windows)
if(WIN32)
    # 链接 user32.lib，用于 SendInput, GetKeyState, GetForegroundWindow 等 Windows API 函数
    target_link_libraries(VirtualKeyboard PRIVATE user32)

    # 可选：将子系统设置为 WINDOWS 以隐藏控制台窗口
    set_target_properties(VirtualKeyboard PROPERTIES WIN32_EXECUTABLE TRUE)

    # 通过 CMake 定义必要的预处理宏，避免在头文件中重复定义和警告
    # WIN32_LEAN_AND_MEAN: 减少 Windows.h 包含的内容
    # NOMINMAX: 避免 Windows.h 定义 min/max 宏，可能与 C++ 标准库冲突
    target_compile_definitions(VirtualKeyboard PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

# 如果你创建了资源文件 (例如 .qrc)，可以在这里添加
# qt_add_resources(VirtualKeyboard "resources.qrc")


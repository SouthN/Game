cmake_minimum_required(VERSION 3.10)  #cmake版本最低3.10
# 获取当前子CMake文件所在目录的完整路径 
get_filename_component(CURRENT_DIR_PATH "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE) 
# 从完整路径中提取目录名称 
get_filename_component(CURRENT_DIR_NAME "${CURRENT_DIR_PATH}" NAME) 
project(${CURRENT_DIR_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 强制标准合规
if(MSVC)
	add_compile_options(/Zc:__cplusplus)
    add_compile_options(/permissive-) # 增强标准符合性，避免MSVC兼容问题
endif()

SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_AUTOMOC ON) 
SET(CMAKE_AUTORCC ON)  
SET(CMAKE_AUTOUIC ON)  

# ==================== 【修改1：新增Qt OpenGL组件查找】====================
# 原有基础上新增 OpenGL 组件，Qt5 中 QOpenGLWidget 依赖该模块
find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL LinguistTools Multimedia MultimediaWidgets Network REQUIRED) 

# 确保链接系统原生OpenGL驱动库，Windows/Linux/macOS 全平台兼容
find_package(OpenGL REQUIRED)

# 配置翻译文件列表
set(TS_FILES
    translations/zh_CN.ts
    translations/en_US.ts
)

# 从ts文件生成qm二进制翻译文件
qt5_add_translation(QM_FILES ${TS_FILES})

include_directories(${PROJECT_SOURCE_DIR}/src)
set(RCC_FILES
    MainWindow.qrc
    BackPack.qrc
    SaveTheApple.qrc
    SpaceWar.qrc
    Audios.qrc
) 
source_group("Resource Files" FILES ${RCC_FILES} ${QM_FILES})

# 主程序入口源文件
set(MAIN_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
)

# 添加子模块编译
add_subdirectory(src/Common)
add_subdirectory(src/Core)
add_subdirectory(src/Games/SaveTheApple)
add_subdirectory(src/Games/SpaceWar)

# 生成可执行程序（WIN32=窗口程序，调试可去掉WIN32保留控制台）
add_executable(${PROJECT_NAME}
    ${MAIN_SOURCES}
    ${RCC_FILES}
    ${QM_FILES} #将qm文件加入可执行程序资源
)

# ==================== 【修改3：新增OpenGL相关库链接】====================
target_link_libraries(${PROJECT_NAME} PRIVATE
    Common
    Core
    SaveTheApple
    SpaceWar
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::OpenGL       # 新增：Qt OpenGL封装库
    OpenGL::GL         # 新增：系统原生OpenGL库
    Qt5::Multimedia        
    Qt5::MultimediaWidgets
    Qt5::Network      # 确认：已经添加了 Qt Network 网络库
)
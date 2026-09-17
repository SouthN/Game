set(CORE_SOURCES
    analyticsmanager.h analyticsmanager.cpp
    audiomanager.h audiomanager.cpp
    backpackdata.h backpackdata.cpp
    backpackdialog.h backpackdialog.cpp
    backpackitemwidget.h backpackitemwidget.cpp
    entityfactory.h
    gameconfigbase.h
    gamecontrollerbase.h gamecontrollerbase.cpp
    gamedatabase.h gamedatabase.cpp
    gameentity.h gameentity.cpp
    gameviewbase.h
    logmanager.h logmanager.cpp
    memorypool.h
    postprocessglwidget.h postprocessglwidget.cpp
    resourcemanager.h resourcemanager.cpp
    singleimagebackground.h singleimagebackground.cpp
    singleton.h
)

# 编译为静态库
add_library(Core STATIC ${CORE_SOURCES})

# 公开头文件目录
target_include_directories(Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 链接依赖：依赖Common基础模块+Qt
target_link_libraries(Core PUBLIC
    Common
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::OpenGL       # 新增：Qt OpenGL封装库
    OpenGL::GL        # 新增：系统原生OpenGL库
    Qt5::Multimedia        
    Qt5::MultimediaWidgets
)
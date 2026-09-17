set(COMMON_SOURCES
    commondefs.h
    inpututils.h
    mathutils.h
    spriteanimation.h
    spriteanimation.cpp
)

# 编译为静态库
add_library(Common STATIC ${COMMON_SOURCES})

# 公开头文件目录，依赖该库的目标自动继承
target_include_directories(Common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 链接Qt依赖
target_link_libraries(Common PUBLIC Qt5::Core Qt5::Widgets Qt5::Gui Qt5::OpenGL OpenGL::GL)
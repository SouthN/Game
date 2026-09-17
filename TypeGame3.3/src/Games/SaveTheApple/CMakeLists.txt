set(SAVETHEAPPLE_SOURCES
    applegamewindow.h applegamewindow.cpp
    Config/applegameconfig.h Config/applegameconfig.cpp
    Controller/applegamecontroller.h Controller/applegamecontroller.cpp
    Model/appleentity.h Model/appleentity.cpp
    Model/applegamedata.h Model/applegamedata.cpp
    View/applegamesettingsdialog.h View/applegamesettingsdialog.cpp
    View/applegameview.h View/applegameview.cpp
    Test/appletestrunner.h Test/appletestrunner.cpp
)

# 编译为静态库
add_library(SaveTheApple STATIC ${SAVETHEAPPLE_SOURCES})

# 公开头文件目录：根目录为公共入口，子目录可通过相对路径引用（如 #include "Config/AppleGameConfig.h"�?
target_include_directories(SaveTheApple PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Config
    ${CMAKE_CURRENT_SOURCE_DIR}/Controller
    ${CMAKE_CURRENT_SOURCE_DIR}/Model
    ${CMAKE_CURRENT_SOURCE_DIR}/View
    ${CMAKE_CURRENT_SOURCE_DIR}/Test
)

# 链接依赖：继承基础模块+Qt核心�?
target_link_libraries(SaveTheApple PUBLIC
    Common
    Core
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
)

# 给Qt的MOC/UIC/RCC自动生成提供目录支持（解决子目录Qt类编译问题）
set_target_properties(SaveTheApple PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# ==========================================
# 单元测试独立工程 (结合 GTest �?GMock)
# ==========================================

# 寻找GTest�?
find_package(GTest REQUIRED)

# 单独�?applegame_test.cpp 创建一个可执行文件目标
add_executable(SaveTheApple_UnitTest Test/applegame_test.cpp)

# 链接主干库以�?GTest/GMock
target_link_libraries(SaveTheApple_UnitTest 
    PRIVATE 
    SaveTheApple   
    Core
    Common
    GTest::gmock
    GTest::gtest 
)

# 开�?CTest 自动发现
include(GoogleTest)
gtest_discover_tests(SaveTheApple_UnitTest)

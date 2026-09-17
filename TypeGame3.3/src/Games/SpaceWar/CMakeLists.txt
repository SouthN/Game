set(SPACEWAR_SOURCES
    planegamewindow.h planegamewindow.cpp
    Config/planegameconfig.h Config/planegameconfig.cpp
    Controller/planegamecontroller.h Controller/planegamecontroller.cpp
    Controller/wordgenerator.h Controller/wordgenerator.cpp
    Model/planeentity.h Model/planeentity.cpp
    Model/planegamedata.h Model/planegamedata.cpp
    Model/playerentity.h Model/playerentity.cpp
    Model/playerattributecomponent.h Model/playerattributecomponent.cpp
    Model/bulletentity.h Model/bulletentity.cpp
    Model/machinegunbulletentity.h Model/machinegunbulletentity.cpp
    Model/enemybulletentity.h Model/enemybulletentity.cpp
    Model/rewardwordentity.h Model/rewardwordentity.cpp
    Model/bossentity.h Model/bossentity.cpp
    View/planegamesettingsdialog.h View/planegamesettingsdialog.cpp
    View/planegameview.h View/planegameview.cpp
    Test/spacetestrunner.h Test/spacetestrunner.cpp
)

# 编译为静态库
add_library(SpaceWar STATIC ${SPACEWAR_SOURCES})

# 公开头文件目录：根目录为公共入口，子目录可通过相对路径引用
target_include_directories(SpaceWar PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Config
    ${CMAKE_CURRENT_SOURCE_DIR}/Controller
    ${CMAKE_CURRENT_SOURCE_DIR}/Model
    ${CMAKE_CURRENT_SOURCE_DIR}/View
    ${CMAKE_CURRENT_SOURCE_DIR}/Test
)

# 链接依赖：继承基础模块+Qt核心 ?
target_link_libraries(SpaceWar PUBLIC
    Common
    Core
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::Network
)

# 给Qt的MOC/UIC/RCC自动生成提供目录支持（解决子目录Qt类编译问题）
set_target_properties(SpaceWar PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# ==========================================
# 单元测试独立工程 (结合 GTest 和 GMock)
# ==========================================
find_package(GTest REQUIRED)

# 单独为 spacewar_test.cpp 创建一个可执行文件目标
add_executable(SpaceWar_UnitTest Test/spacewar_test.cpp)

# 链接主干库以及 GTest/GMock
target_link_libraries(SpaceWar_UnitTest 
    PRIVATE 
    SpaceWar   
    Core
    Common
    GTest::gmock
    GTest::gtest 
)

# 开启 CTest 自动发现
include(GoogleTest)
gtest_discover_tests(SpaceWar_UnitTest)

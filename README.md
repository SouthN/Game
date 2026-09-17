# Game

基于 Qt 5 / C++17 的打字游戏项目，收录最终版本 **TypeGame 3.3**。

## 游戏内容

- **拯救苹果（SaveTheApple）**
- **太空大战（SpaceWar）**
- 公共游戏框架、背包、音频、日志、OpenGL 效果及中英文翻译资源

## 目录结构

```text
TypeGame3.3/
├── CMakeLists.txt     CMake 构建入口
├── src/              C++ 源码及测试
│   ├── Common/       公共工具
│   ├── Core/         游戏核心框架
│   └── Games/        两个游戏模块
├── assets/           图片、音频及界面样式
├── translations/     中英文翻译
└── *.qrc             Qt 资源清单
```

## 构建

需要 C++17 编译器、CMake、Qt 5 和 GoogleTest / GoogleMock。Qt 组件包括 Core、Widgets、Gui、OpenGL、LinguistTools、Multimedia、MultimediaWidgets 和 Network，并需系统 OpenGL 开发库。

可在 Qt Creator 中打开 `TypeGame3.3/CMakeLists.txt`，选择合适的 Qt 5 Kit，并配置 GoogleTest / GoogleMock 的安装路径。当前构建配置会同时构建单元测试，因此测试依赖也需要安装。

命令行示例（将路径替换为本机实际路径，并使用匹配的编译器环境）：

```sh
cmake -S TypeGame3.3 -B build -DCMAKE_PREFIX_PATH="<Qt5安装目录>;<GoogleTest安装目录>"
cmake --build build --config Release
```

本仓库保留原项目的源码和资源，未附带 Qt、GoogleTest 等外部依赖或编译后的可执行程序。

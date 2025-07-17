# Qt项目在VS Code中的使用说明

## 环境配置

项目已配置为使用位于 `/Users/yhx/Qt/6.9.1/macos` 的Qt 6.9.1安装。

## 构建和运行

### 方法1：使用VS Code任务
1. 打开命令面板 (`Cmd+Shift+P`)
2. 运行任务：
   - `Tasks: Run Task` → `cmake-configure` (配置项目)
   - `Tasks: Run Task` → `cmake-build` (构建项目) 
   - `Tasks: Run Task` → `run-app` (运行应用)

### 方法2：使用终端命令
```bash
# 配置项目
/Users/yhx/Qt/Tools/CMake/CMake.app/Contents/bin/cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/Users/yhx/Qt/6.9.1/macos .

# 构建项目
/Users/yhx/Qt/Tools/CMake/CMake.app/Contents/bin/cmake --build build --config Debug

# 运行应用
./build/whatever.app/Contents/MacOS/whatever
```

### 方法3：使用环境设置脚本
```bash
# 设置Qt环境变量
source ./setup_qt_env.sh

# 然后可以直接使用cmake
cmake -B build -DCMAKE_BUILD_TYPE=Debug .
cmake --build build --config Debug
./build/whatever.app/Contents/MacOS/whatever
```

## 调试

1. 设置断点
2. 按 `F5` 或使用命令面板 `Debug: Start Debugging`
3. 选择 "Debug Qt App" 配置

## 清理构建

运行任务：`Tasks: Run Task` → `clean`

## 扩展建议

已安装的有用扩展：
- C/C++ Extension Pack
- CMake Tools
- Qt for Python (提供Qt相关语法高亮)

## 项目结构

- `mainwindow.cpp/h` - 主窗口实现
- `mainwindow.ui` - UI界面设计文件
- `main.cpp` - 应用程序入口点
- `CMakeLists.txt` - CMake构建配置
- `assets/` - 资源文件
- `.vscode/` - VS Code配置文件
  - `tasks.json` - 构建任务配置
  - `launch.json` - 调试配置
  - `c_cpp_properties.json` - IntelliSense配置
  - `settings.json` - 工作区设置

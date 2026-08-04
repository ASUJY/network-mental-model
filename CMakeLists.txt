cmake_minimum_required(VERSION 3.28)    # CMake最低版本要求3.10
project(network-mental)                     # 工程名

set(CMAKE_CXX_STANDARD 11)              # 全局使用C++11标准编译所有代码
set(CMAKE_CXX_STANDARD_REQUIRED ON)     # 编译器必须支持C++11以上，否则不进行编译
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # 导出 compile_commands.json 文件

# 统一二进制产物输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_subdirectory(modules)
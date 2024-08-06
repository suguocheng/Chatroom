cmake_minimum_required(VERSION 3.10)
project(Client)

# 指定 C++ 标准
set(CMAKE_CXX_STANDARD 17)

# 添加源文件
add_executable(MyExecutable main.cpp)

# 如果有其他库，添加库链接
# target_link_libraries(MyExecutable SomeLibrary)


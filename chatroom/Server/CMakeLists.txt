cmake_minimum_required(VERSION 3.10)  # 指定 CMake 的最低版本
project(MyServerProject)  # 定义项目名称

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 定义源文件
set(SOURCES
    Server.cpp
    ../ThreadPool/ThreadPool.cpp
    RedisManager.cpp
    main.cpp
    ../log/mars_logger.cc
)

# 添加可执行文件
add_executable(Server ${SOURCES})

target_link_libraries(Server
    -lhiredis
    -ljsoncpp
    -lfmt
    -lstdc++fs
)
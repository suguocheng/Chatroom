cmake_minimum_required(VERSION 3.10)
project(MyClientProject)

# 指定 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找所有源文件
set(SOURCES
    Client.cpp
    ../ThreadPool/ThreadPool.cpp
    Account_UI.cpp
    SendJson.cpp
    Home_UI.cpp
    main.cpp
    ../log/mars_logger.cc
)

# 添加可执行文件
add_executable(Client ${SOURCES})

# 链接库
target_link_libraries(Client
    -ljsoncpp
    -lfmt
    -lstdc++fs
)




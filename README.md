# 一个简易聊天室项目

### 概述

这个项目是一个简易聊天室应用，旨在提供一个基本的客户端-服务器（C/S）聊天系统。用户可以通过客户端连接到服务器，实现实时的消息发送和接收。该项目展示了如何使用 C++ 和各种现代技术（如 Redis 数据库和 JSON 数据格式）来构建一个具有实际用途的聊天系统。

#### 主要特点：

- **客户端和服务器实现**：包含独立的客户端和服务器端代码，客户端用于用户交互，服务器处理消息传递和数据存储。
- **多线程支持**：服务器使用线程池来处理并发的客户端连接，提高了系统的响应能力和吞吐量。
- **Redis 数据库集成**：服务器利用 Redis 进行用户数据管理和消息存储，提升了数据的存取效率。
- **JSON 数据格式**：使用 JSON 格式来封装和解析聊天数据，确保了数据的结构化和可读性。
- **自定义日志系统**：集成了自定义的日志库用于记录系统操作和调试信息。
- **基于 epoll 的 I/O 事件处理**：服务器使用 epoll 机制进行高效的 I/O 事件通知，以处理大量的并发连接。

### 功能



![](https://github.com/suguocheng/Chatroom/客户端功能图示.png)



### 项目结构 

```
Chatroom/
│   chatroom/
│   │
│   ├── Server/                   		# 服务器目录
│   │   ├── main.cpp            		# 主程序入口
│   │   ├── Server.cpp          		# 服务器实现
│   │   ├── Server.h   					# 服务器接口
│   │	├── RedisManager.cpp          	# redis 操作实现
│   │   ├── RedisManager.h 				# redis 操作接口
│   │	├── CMakeLists.txt          	# CMake 构建脚本
│   │   └── file_buf/  					# 文件存储目录
│   │		└── .gitignore				# Git 忽略文件
│   │
│   ├── Client/     					# 客户端目录
│   │   ├── main.cpp            		# 主程序入口
│   │   ├── Client.cpp 					# 客户端实现
│   │   ├── Client.h            		# 客户端接口
│   │   ├── Account_UI.cpp 				# 账号 UI 实现
│   │   ├── Account_UI.h            	# 帐号 UI 接口
│   │   ├── Home_UI.cpp 				# 主页 UI 实现
│   │   ├── Home_UI.h  					# 主页 UI 接口
│   │   ├── SendJson.cpp 				# 发送 JSON 数据实现
│   │   ├── SendJson.h  				# 发送 JSON 数据接口
│   │   ├── CMakeLists.txt 				# CMake 构建脚本
│   │   └── file_buf/ 					# 文件存储目录
│   │		└── .gitignore				# Git 忽略文件
│   │
│   ├── ThreadPool/             		# 线程池目录
│   │   ├── ThreadPool.cpp     			# 线程池实现
│   │   └── ThreadPool.h     			# 线程池接口
│   │
│   └── log/                  			# marsevilspirit 的日志库
│       ├── mars_logger.cc 
│       ├── mars_logger.hh
│       └── logconf.json               
│            
└── README.md                    	# 项目说明文档 
```

### 技术栈

开发环境：GNU/Linux 环境( ubuntu )

网络结构模式：C/S 模型

开发语言：C++

编译工具：CMake

数据库：Redis

数据格式：JSON

I/O事件通知机制：epoll

### 安装与使用

```shell
git clone git@github.com:suguocheng/Chatroom.git

确认安装cmake以及必需库

服务器编译与启动
cd Chatroom/chatroom/Server
mkdir build
cd build
cmake ..
make
./Server 端口号

客户端编译与启动
cd Chatroom/chatroom/Client
mkdir build
cd build
cmake ..
make
./Client ip地址 端口号
```

### 联系信息

如果你有任何问题或建议，请通过以下方式联系我：

电子邮件：su1799718517@gmail.com

Github：https://github.com/suguocheng
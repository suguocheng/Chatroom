# 一个简易聊天室项目

### 概述



### 功能



![](/home/pluto/Downloads/未命名绘图.drawio (1).png)



### 项目结构 

```
chatroom/
│
├── Server/                   		# 服务器目录
│   ├── main.cpp            		# 主程序入口
│   ├── Server.cpp          		# 服务器实现
│   ├── Server.h   					# 服务器接口
│	├── RedisManager.cpp          	# redis 操作实现
│   ├── RedisManager.h 				# redis 操作接口
│	├── CMakeLists.txt          	# CMake 构建脚本
│   └── file_buf/  					# 文件存储目录
│		└── .gitignore				# Git 忽略文件
│
├── Client/     					# 客户端目录
│   ├── main.cpp            		# 主程序入口
│   ├── Client.cpp 					# 客户端实现
│   ├── Client.h            		# 客户端接口
│   ├── Account_UI.cpp 				# 账号 UI 实现
│   ├── Account_UI.h            	# 帐号 UI 接口
│   ├── Home_UI.cpp 				# 主页 UI 实现
│   ├── Home_UI.h  					# 主页 UI 接口
│   ├── SendJson.cpp 				# 发送 JSON 数据实现
│   ├── SendJson.h  				# 发送 JSON 数据接口
│   ├── CMakeLists.txt 				# CMake 构建脚本
│   └── file_buf/ 					# 文件存储目录
│		└── .gitignore				# Git 忽略文件
│
├── ThreadPool/             		# 线程池目录
│   ├── ThreadPool.cpp     			# 线程池实现
│   └── ThreadPool.h     			# 线程池接口
│
├── log/                  			# marsevilspirit 的日志库
│   ├── mars_logger.cc 
│   ├── mars_logger.hh
│   └── logconf.json               
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
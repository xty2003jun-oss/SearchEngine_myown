# 在项目根目录配置并编译，产物放进 build/，源码目录保持干净：
#   cmake -S . -B build
#   cmake --build build
#
# 启动时的当前目录必须是项目根目录，词典和网页库用的是相对路径：
#   ./build/server
#   ./build/client
# 离线程序的路径写成 ../corpus、../dicts，所以要在 offline 目录里启动：
#   cd offline && ../build/offline

cmake_minimum_required(VERSION 3.16)
project(SearchEngine_myown LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)          # 生成 compile_commands.json，编辑器据此找头文件

add_compile_options(-Wall -Wextra)
add_compile_definitions(LOG4CPP_FIX_ERROR_COLLISION)  # 避免 log4cpp 和系统宏重名

set(CMAKE_BUILD_RPATH "/usr/local/lib")        # 运行时到这里找 liblog4cpp.so

set(JIEBA_INCLUDE "/home/xty/SearchEngine/third_party/cppjieba/include")
set(SIMHASH_INCLUDE "/home/xty/SearchEngine/third_party/simhash/include")
set(TINYXML_DIR "/home/xty/SearchEngine/Server(服务器)/DirectoryHandler(预处理库)")

# 源码里仍是 #include "TcpServer.h" 这种不带目录的写法，所以把各子目录加进搜索路径
set(SERVER_INCLUDES
    ${CMAKE_SOURCE_DIR}/src/net
    ${CMAKE_SOURCE_DIR}/src/pool
    ${CMAKE_SOURCE_DIR}/src/cache
    ${CMAKE_SOURCE_DIR}/src/search
    ${CMAKE_SOURCE_DIR}/src/log
    ${JIEBA_INCLUDE}
    /usr/local/include
)

find_package(Threads REQUIRED)

# 在线服务：网络、线程池、日志、关键词推荐、网页检索
add_executable(server
    src/server.cpp
    src/net/InetAddress.cpp
    src/net/Socket.cpp
    src/net/Message.cpp
    src/net/Acceptor.cpp
    src/net/EventLoop.cpp
    src/net/TcpConnection.cpp
    src/net/TcpServer.cpp
    src/pool/TaskQueue.cpp
    src/pool/ThreadPool.cpp
    src/log/log.cpp
    src/search/KeywordRecommander.cpp
    src/search/WebPageSearcher.cpp
)
target_include_directories(server PRIVATE ${SERVER_INCLUDES})
target_link_directories(server PRIVATE /usr/local/lib)
target_link_libraries(server PRIVATE log4cpp Threads::Threads)

# 交互客户端只用地址、套接字和协议，不链检索和日志
add_executable(client
    client/client.cpp
    src/net/InetAddress.cpp
    src/net/Socket.cpp
    src/net/Message.cpp
)
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/src/net)

# 离线建库。tinyxml2 没有单独安装，直接编译那一份源文件
add_executable(offline
    offline/main.cpp
    offline/DirectoryScanner.cpp
    offline/KeywordProcessor.cpp
    offline/PageProcessor.cpp
    "${TINYXML_DIR}/tinyxml2.cc"
)
target_include_directories(offline PRIVATE
    ${CMAKE_SOURCE_DIR}/offline
    ${JIEBA_INCLUDE}
    ${SIMHASH_INCLUDE}
    "${TINYXML_DIR}"
)

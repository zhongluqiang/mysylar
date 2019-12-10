cmake_minimum_required(VERSION 3.10)
project(mysylar)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
#指定编译选项，-rdynamic用于将所有符号都加入到符号表中，便于使用dlopen或者backtrace追踪到符号
set(CMAKE_CXX_FLAGS "-rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror")
#-Wno-unused-function，不要警告未使用函数
#-Wno-builtin-macro-redefined，不要警告内置宏重定义，用于重定义内置的__FILE__宏
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.)

set(
    LIB_SRC

    sylar/thread.h 
    sylar/thread.cpp
    sylar/config.h 
    sylar/config.cpp
    sylar/log.h 
    sylar/log.cpp
    sylar/singleton.h 
    sylar/util.h 
    sylar/util.cpp
    sylar/fiber.h 
    sylar/fiber.cpp 
    sylar/scheduler.h 
    sylar/scheduler.cpp
    sylar/timer.h 
    sylar/timer.cpp 
    sylar/hook_sys_call.h 
    sylar/hook_sys_call.cpp 
    sylar/address.h 
    sylar/address.cpp 
    sylar/socket.h 
    sylar/socket.cpp
    sylar/http/http.h 
    sylar/http/http.cpp
    sylar/tcp_server.h 
    sylar/tcp_server.cpp 
    sylar/http/http11_common.h 
    sylar/http/http11_parser.h
    sylar/http/http11_parser.cpp 
    sylar/http/httpclient_parser.h 
    sylar/http/httpclient_parser.cpp
    sylar/http/http_parser.h 
    sylar/http/http_parser.cpp 
    sylar/endian.h 
    )

set(
    LIBS 

    sylar 
    dl
    pthread
    yaml-cpp 
)

add_library(sylar SHARED ${LIB_SRC})
#该函数将sylar这个目标的全部源代码内的__FILE__宏进行重新定义，去掉绝对路径，改用相对路径
#force_redefine_file_macro_for_sources(sylar)

#日志类测试
add_executable(test_log tests/test_log.cpp)
#force_redefine_file_macro_for_sources(test_log)
add_dependencies(test_log sylar)
target_link_libraries(test_log ${LIBS})

#yaml-cpp库测试
add_executable(test_yaml tests/test_yaml.cpp)
#force_redefine_file_macro_for_sources(test_yaml)
target_link_libraries(test_yaml ${LIBS})

#配置管理类测试
add_executable(test_config tests/test_config.cpp)
#force_redefine_file_macro_for_sources(test_config)
add_dependencies(test_config sylar)
target_link_libraries(test_config ${LIBS})

#线程类测试
add_executable(test_thread tests/test_thread.cpp)
#force_redefine_file_macro_for_sources(test_thread)
add_dependencies(test_thread sylar)
target_link_libraries(test_thread ${LIBS})

#辅助接口测试
add_executable(test_util tests/test_util.cpp)
#force_redefine_file_macro_for_sources(test_util)
add_dependencies(test_util sylar)
target_link_libraries(test_util ${LIBS})

#ucontext接口测试
add_executable(test_ucontext tests/test_ucontext.cpp)

#fiber类测试
add_executable(test_fiber tests/test_fiber.cpp)
#force_redefine_file_macro_for_sources(test_fiber)
add_dependencies(test_fiber sylar)
target_link_libraries(test_fiber ${LIBS})

#scheduler类测试
add_executable(test_scheduler tests/test_scheduler.cpp)
#force_redefine_file_macro_for_sources(test_scheduler)
add_dependencies(test_scheduler sylar)
target_link_libraries(test_scheduler ${LIBS})

#epoll接口测试
add_executable(test_epoll tests/test_epoll.cpp)

#IO协程调度测试
add_executable(test_io_schedule tests/test_io_schedule.cpp)
#force_redefine_file_macro_for_sources(test_io_schedule)
add_dependencies(test_io_schedule sylar)
target_link_libraries(test_io_schedule ${LIBS})

#timerfd接口测试
add_executable(test_timerfd tests/test_timerfd.cpp)

#定时器测试
add_executable(test_timer_schedule tests/test_timer_schedule.cpp)
#force_redefine_file_macro_for_sources(test_timer_schedule)
add_dependencies(test_timer_schedule sylar)
target_link_libraries(test_timer_schedule ${LIBS})

#hook系统调用测试
add_executable(test_hook tests/test_hook.cpp)
#force_redefine_file_macro_for_sources(test_hook)
add_dependencies(test_hook sylar)
target_link_libraries(test_hook ${LIBS})

#address类测试
add_executable(test_address tests/test_address.cpp)
#force_redefine_file_macro_for_sources(test_address)
add_dependencies(test_address sylar)
target_link_libraries(test_address ${LIBS})

#socket类测试
add_executable(test_socket tests/test_socket.cpp)
#force_redefine_file_macro_for_sources(test_socket)
add_dependencies(test_socket sylar)
target_link_libraries(test_socket ${LIBS})

#http类测试
add_executable(test_http tests/test_http.cpp)
#force_redefine_file_macro_for_sources(test_http)
add_dependencies(test_http sylar)
target_link_libraries(test_http ${LIBS})

#TcpServer类测试
add_executable(test_tcpserver tests/test_tcpserver.cpp)
#force_redefine_file_macro_for_sources(test_tcpserver)
add_dependencies(test_tcpserver sylar)
target_link_libraries(test_tcpserver ${LIBS})

#http解析类测试
add_executable(test_http_parser tests/test_http_parser.cpp)
#force_redefine_file_macro_for_sources(test_http_parser)
add_dependencies(test_http_parser sylar)
target_link_libraries(test_http_parser ${LIBS})

#大小端测试
add_executable(test_endian tests/test_endian.cpp)
#force_redefine_file_macro_for_sources(test_endian)
add_dependencies(test_endian sylar)
target_link_libraries(test_endian ${LIBS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
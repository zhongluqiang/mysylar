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
    )

set(
    LIBS 

    sylar 
    yaml-cpp 
    pthread
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

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
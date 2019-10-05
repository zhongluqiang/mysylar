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

    sylar/log.h 
    sylar/log.cpp
    sylar/singleton.h 
    sylar/util.h 
    sylar/util.cpp
    )

add_library(sylar SHARED ${LIB_SRC})
#该函数将sylar这个目标的全部源代码内的__FILE__宏进行重新定义，去掉绝对路径，改用相对路径
force_redefine_file_macro_for_sources(sylar)

#日志类测试
add_executable(test_log tests/test_log.cpp)
force_redefine_file_macro_for_sources(test_log)
add_dependencies(test_log sylar)
target_link_libraries(test_log sylar)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
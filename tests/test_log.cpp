#include "sylar/log.h"
#include <iostream>

int main() {
    sylar::Logger::ptr logger(new sylar::Logger);

    //[日期时间]  文件名:行号  线程号:协程号  日志级别  日志内容\n
    sylar::LogFormatter::ptr fmt(
        new sylar::LogFormatter("[%d]  %f：%l  %t:%F  %p  %m%n"));

    sylar::FileLogAppender::ptr file_appender(
        new sylar::FileLogAppender("./log.txt"));
    file_appender->setFormater(fmt);
    file_appender->setLevel(sylar::LogLevel::ERROR);
    logger->addAppender(file_appender);

    sylar::StdoutLogAppender::ptr stdout_appender(
        new sylar::StdoutLogAppender());
    stdout_appender->setFormater(fmt);
    logger->addAppender(stdout_appender);

    std::cout << "hello sylar log" << std::endl;

    SYLAR_LOG_DEBUG(logger) << "test macro  debug";
    SYLAR_LOG_INFO(logger) << "test macro info";
    SYLAR_LOG_WARN(logger) << "test macro logger";
    SYLAR_LOG_ERROR(logger) << "test macro error";
    SYLAR_LOG_FATAL(logger) << "test macro fatal";

    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "test macro SYLAR_LOG_ROOT";

    return 0;
}
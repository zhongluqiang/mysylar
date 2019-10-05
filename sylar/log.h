#ifndef MYSYLAR_LOG_H
#define MYSYLAR_LOG_H

#include "singleton.h"
#include "util.h"
#include <cstdarg> //for va_list
#include <cstdint>
#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <sstream> //for stringstream
#include <string>
#include <vector>

#define SYLAR_LOG_LEVEL(logger, level)                                         \
    if (logger->getLevel() <= level)                                           \
    sylar::LogEventWrap(                                                       \
        sylar::LogEvent::ptr(new sylar::LogEvent(                              \
            logger, level, __FILE__, __LINE__, 0, sylar::GetThreadId(),        \
            sylar::GetFiberId(), time(0))))                                    \
        .getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

namespace sylar {

class Logger;

//日志级别
class LogLevel {
public:
    enum Level {
        UNKNOWN = 0,
        DEBUG   = 1,
        INFO    = 2,
        WARN    = 3,
        ERROR   = 4,
        FATAL   = 5
    };

    static const char *ToString(LogLevel::Level level);
};

//日志事件
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr; //定义智能指针类型

    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
             const char *file, int32_t line, uint32_t elapse,
             uint32_t thread_id, uint32_t fiber_id, uint64_t time);

    const char *getFile() const { return m_file; }
    int32_t getLine() const { return m_line; }
    uint32_t getElapse() const { return m_elapse; }
    uint32_t getThreadId() const { return m_threadId; }
    uint32_t getFiberId() const { return m_fiberId; }
    uint64_t getTime() const { return m_time; }
    std::string getContent() const { return m_ss.str(); }
    std::shared_ptr<Logger> getLogger() const { return m_logger; }
    LogLevel::Level getLevel() const { return m_level; }
    std::stringstream &getSS() { return m_ss; }
    void format(const char *fmt, ...);
    void format(const char *fmt, va_list al);

private:
    const char *m_file  = nullptr; //文件名
    int32_t m_line      = 0;       //行号
    uint32_t m_elapse   = 0;       //程序启动到现在的毫秒数
    uint32_t m_threadId = 0;       //线程id
    uint32_t m_fiberId  = 0;       //协程id
    uint64_t m_time     = 0;       //时间戳
    std::stringstream m_ss;        //日志内容

    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;
};

class LogEventWrap {
public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();

    LogEvent::ptr getEvent() const { return m_event; }
    std::stringstream &getSS() { return m_event->getSS(); }

private:
    LogEvent::ptr m_event;
};

//日志格式器
class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string &pattern);
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level,
                       LogEvent::ptr);
    void init();

public:
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem() {}

        // format方法由子类实现
        virtual void format(std::ostream &os, std::shared_ptr<Logger> logger,
                            LogLevel::Level level, LogEvent::ptr event) = 0;
    };

private:
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_items;
};

//日志输出地
class LogAppender {
public:
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender() {}

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event) = 0;
    void setFormater(LogFormatter::ptr val) { m_formatter = val; }
    LogFormatter::ptr getFormatter() const { return m_formatter; }
    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level val) { m_level = val; }

protected: //继承的子类需要访问这些成员，只能声明成protected类型
    LogLevel::Level m_level = LogLevel::DEBUG;
    LogFormatter::ptr m_formatter;
};

//输出到控制台的Appender
class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
             LogEvent::ptr event) override;
};

//输出到文件的Appender
class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string &filename);
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
             LogEvent::ptr event) override;
    bool reopen();

private:
    std::string m_filename;
    std::ofstream m_filestream;
};

// Logger类继承至enable_shared_from_this以便于获取自身的智能指针
class Logger : public std::enable_shared_from_this<Logger> {
public:
    typedef std::shared_ptr<Logger> ptr;

    Logger(const std::string &name = "root");
    void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level val) { m_level = val; }

    const std::string &getName() const { return m_name; }

private:
    std::string m_name;
    LogLevel::Level m_level;
    std::list<LogAppender::ptr> m_appenders;
    LogFormatter::ptr m_formatter;
};

class LoggerManager {
public:
    LoggerManager() {
        m_root.reset(new Logger("root"));
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
    }
    Logger::ptr getLogger(const std::string &name) {
        auto it = m_loggers.find(name);
        return it == m_loggers.end() ? m_root : it->second;
    }
    void init();
    Logger::ptr getRoot() const { return m_root; }

private:
    std::map<std::string, Logger::ptr> m_loggers;
    Logger::ptr m_root;
};

typedef sylar::Singleton<LoggerManager> LoggerMgr;

#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()

} // end namespace sylar

#endif // MYSYLAR_LOG_H

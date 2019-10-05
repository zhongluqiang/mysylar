#include "log.h"
#include <ctime>
#include <functional>
#include <iostream>
#include <tuple>

namespace sylar {

const char *LogLevel::ToString(LogLevel::Level level) {
    switch (level) {
#define XX(name)                                                               \
    case LogLevel::name:                                                       \
        return #name;                                                          \
        break;

        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);
#undef XX
    default:
        return "UNKNOWN";
    }

    return "UNKNOWN";
}

LogEvent::LogEvent(std::shared_ptr<Logger> logger, sylar::LogLevel::Level level,
                   const char *file, int32_t line, uint32_t elapse,
                   uint32_t thread_id, uint32_t fiber_id, uint64_t time)
    : m_file(file)
    , m_line(line)
    , m_elapse(elapse)
    , m_threadId(thread_id)
    , m_fiberId(fiber_id)
    , m_time(time)
    , m_logger(logger)
    , m_level(level) {}

void LogEvent::format(const char *fmt, ...) {
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}

void LogEvent::format(const char *fmt, va_list al) {
    char *buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if (len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}

LogEventWrap::LogEventWrap(sylar::LogEvent::ptr e)
    : m_event(e) {}

LogEventWrap::~LogEventWrap() {
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

LogFormatter::LogFormatter(const std::string &pattern)
    : m_pattern(pattern) {
    init();
}

//日志事件内容格式化
class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << logger->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string &format = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        struct tm tm;
        std::string format = "%Y-%m-%d %H:%M:%S";
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), format.c_str(), &tm);
        os << buf;
    }
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
    FilenameFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string &str)
        : m_string(str) {}

    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << m_string;
    }

private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string &str = "") {}
    void format(std::ostream &os, std::shared_ptr<Logger> logger,
                LogLevel::Level level, LogEvent::ptr event) override {
        os << "\t";
    }
};

//解析pattern中的格式项
//%d%T%t%T%F%T%p%T%c%T%f:%l%T%m%n
void LogFormatter::init() {
    static std::map<char,
                    std::function<FormatItem::ptr(const std::string &str)>>
        s_format_items = {
#define XX(str, C)                                                             \
    {                                                                          \
        str,                                                                   \
            [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); } \
    }

            XX('m', MessageFormatItem),  XX('p', LevelFormatItem),
            XX('r', ElapseFormatItem),   XX('c', NameFormatItem),
            XX('t', ThreadIdFormatItem), XX('n', NewLineFormatItem),
            XX('d', DateTimeFormatItem), XX('f', FilenameFormatItem),
            XX('l', LineFormatItem),     XX('T', TabFormatItem),
            XX('F', FiberIdFormatItem),
#undef XX
        };

    // char, type
    std::vector<std::pair<char, int>> vec;

    for (size_t i = 0; i < m_pattern.size(); ++i) {
        if (i == m_pattern.size() - 1) {
            vec.push_back(std::make_pair(m_pattern[i], 0));
            break;
        }

        if (m_pattern[i] == '%') {
            //将后一个字符作为转义字符添加到列表
            vec.push_back(std::make_pair(m_pattern[i + 1], 1));
            ++i;
        } else {
            vec.push_back(std::make_pair(m_pattern[i], 0));
        }
    }

    for (auto &i : vec) {
        int second = i.second;
        std::string first(1, i.first);
        if (second == 0) {
            //非转义字符
            m_items.push_back(FormatItem::ptr(new StringFormatItem(first)));
        } else {
            auto it = s_format_items.find(i.first);
            if (it != s_format_items.end()) {
                m_items.push_back(it->second(first));
            } else {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(first)));
            }
        }
    }
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger,
                                 LogLevel::Level level, LogEvent::ptr event) {
    std::stringstream ss;
    for (auto &i : m_items) {
        i->format(ss, logger, level, event);
    }
    return ss.str();
}

void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level,
                            LogEvent::ptr event) {
    if (level >= LogAppender::m_level) {
        std::cout << LogAppender::m_formatter->format(logger, level, event);
    }
}

bool FileLogAppender::reopen() {
    if (m_filestream) {
        m_filestream.close();
    }

    m_filestream.open(m_filename, std::ios::app);
    return !!m_filestream;
}

FileLogAppender::FileLogAppender(const std::string &filename)
    : m_filename(filename) {
    reopen();
}

void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level,
                          LogEvent::ptr event) {
    if (level >= LogAppender::m_level) {
        m_filestream << LogAppender::m_formatter->format(logger, level, event);
    }
}
Logger::Logger(const std::string &name)
    : m_name(name)
    , m_level(LogLevel::DEBUG) {
    m_formatter.reset(
        //时间日期 线程号 协程号
        new LogFormatter("%d%T%t%T%F%T%p%T%c%T%f:%l%T%m%n"));
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
    if (level > m_level) {
        auto self = shared_from_this();
        for (auto &i : m_appenders) {
            i->log(self, level, event);
        }
    }
}

void Logger::debug(sylar::LogEvent::ptr event) { log(LogLevel::DEBUG, event); }

void Logger::info(sylar::LogEvent::ptr event) { log(LogLevel::INFO, event); }

void Logger::warn(sylar::LogEvent::ptr event) { log(LogLevel::WARN, event); }

void Logger::error(sylar::LogEvent::ptr event) { log(LogLevel::ERROR, event); }

void Logger::fatal(sylar::LogEvent::ptr event) { log(LogLevel::FATAL, event); }

void Logger::addAppender(sylar::LogAppender::ptr appender) {
    if (!appender->getFormatter()) {
        appender->setFormater(m_formatter);
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(sylar::LogAppender::ptr appender) {
    for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
        if (*it == appender) {
            m_appenders.erase(it);
        }
    }
}

} // end namespace sylar
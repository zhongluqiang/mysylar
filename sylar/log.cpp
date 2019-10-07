#include "log.h"
#include "config.h"
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

LogLevel::Level LogLevel::FromString(const std::string &str) {
#define XX(level, v)                                                           \
    if (str == #v) {                                                           \
        return LogLevel::level;                                                \
    }
    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);
    return LogLevel::UNKNOWN;
#undef XX
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
    int len   = vasprintf(&buf, fmt, al);
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
        os << event->getLogger()->getName();
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
        time_t time        = event->getTime();
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

void LogAppender::setFormater(LogFormatter::ptr val) {
    MutexType::Lock lock(m_mutex);
    m_formatter = val;
    if (m_formatter) {
        m_hasFormatter = true;
    } else {
        m_hasFormatter = false;
    }
}

LogFormatter::ptr LogAppender::getFormatter() {
    MutexType::Lock lock(m_mutex);
    return m_formatter;
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

std::string StdoutLogAppender::toYamlString() {
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if (m_level != LogLevel::UNKNOWN) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if (m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

bool FileLogAppender::reopen() {
    MutexType::Lock lock(m_mutex);
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
        MutexType::Lock lock(m_mutex);
        m_filestream << LogAppender::m_formatter->format(logger, level, event);
    }
}

std::string FileLogAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if (m_level != LogLevel::UNKNOWN) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if (m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

Logger::Logger(const std::string &name)
    : m_name(name)
    , m_level(LogLevel::DEBUG) {
    m_formatter.reset(
        //时间日期 线程号 协程号
        new LogFormatter("%d%T%t%T%F%T%p%T%c%T%f:%l%T%m%n"));
}

// 某个logger没有appender时，使用root logger进行输出 ？？
void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
    if (level > m_level) {
        MutexType::Lock lock(m_mutex);
        auto self = shared_from_this();
        if (!m_appenders.empty()) {
            for (auto &i : m_appenders) {
                i->log(self, level, event);
            }
        } else if (m_root) {
            m_root->log(level, event);
        }
    }
}

void Logger::debug(sylar::LogEvent::ptr event) { log(LogLevel::DEBUG, event); }

void Logger::info(sylar::LogEvent::ptr event) { log(LogLevel::INFO, event); }

void Logger::warn(sylar::LogEvent::ptr event) { log(LogLevel::WARN, event); }

void Logger::error(sylar::LogEvent::ptr event) { log(LogLevel::ERROR, event); }

void Logger::fatal(sylar::LogEvent::ptr event) { log(LogLevel::FATAL, event); }

void Logger::addAppender(sylar::LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);
    if (!appender->getFormatter()) {
        appender->setFormater(m_formatter);
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(sylar::LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);
    for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
        if (*it == appender) {
            m_appenders.erase(it);
        }
    }
}

void Logger::clearAppenders() {
    MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}

void Logger::setFormatter(LogFormatter::ptr val) {
    MutexType::Lock lock(m_mutex);

    m_formatter = val;
    for (auto &i : m_appenders) {
        MutexType::Lock ll(i->m_mutex);
        if (!i->m_hasFormatter) {
            i->m_formatter = m_formatter;
        }
    }
}

void Logger::setFormatter(const std::string &val) {
    LogFormatter::ptr new_val(new LogFormatter(val));
    if (new_val->isError()) {
        std::cout << "Logger set Formatter name=" << m_name << " value=" << val
                  << " invalid formatter" << std::endl;
        return;
    }
    m_formatter = new_val;
}

LogFormatter::ptr Logger::getFormatter() {
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

std::string Logger::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["name"] = m_name;
    if (m_level != LogLevel::UNKNOWN) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if (m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    for (auto &i : m_appenders) {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }

    std::stringstream ss;
    ss << node;
    return ss.str();
}

Logger::ptr LoggerManager::getLogger(const std::string &name) {
    MutexType::Lock lock(m_mutex);
    auto it = m_loggers.find(name);
    if (it != m_loggers.end()) {
        return it->second;
    }

    Logger::ptr logger(new Logger(name));
    logger->m_root  = m_root;
    m_loggers[name] = logger;
    return logger;
}

std::string LoggerManager::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    for (auto &i : m_loggers) {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }

    std::stringstream ss;
    ss << node;
    return ss.str();
}

struct LogAppenderDefine {
    int type              = 0; // 1:File, 2:Stdout
    LogLevel::Level level = LogLevel::UNKNOWN;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine &oth) const {
        return type == oth.type && level == oth.level &&
               formatter == oth.formatter && file == oth.formatter;
    }
};

struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOWN;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine &oth) const {
        return name == oth.name && level == oth.level &&
               formatter == oth.formatter && appenders == oth.appenders;
    }

    //使用set存储的类型必须实现"<"运行符
    bool operator<(const LogDefine &oth) const { return name < oth.name; }
};

template <> class LexicalCast<std::string, std::set<LogDefine>> {
public:
    std::set<LogDefine> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        std::set<LogDefine> ld_set;

        for (size_t i = 0; i < node.size(); ++i) {
            auto n = node[i];
            if (!n["name"].IsDefined()) {
                std::cout << "log config error: name is null, " << n
                          << std::endl;
                continue;
            }

            LogDefine ld;
            ld.name  = n["name"].as<std::string>();
            ld.level = LogLevel::FromString(
                n["level"].IsDefined() ? n["level"].as<std::string>() : "");

            if (n["formatter"].IsDefined()) {
                ld.formatter = n["formatter"].as<std::string>();
            }

            if (n["appenders"].IsDefined()) {
                for (size_t x = 0; x < n["appenders"].size(); ++x) {
                    auto a = n["appenders"][x];
                    if (!a["type"].IsDefined()) {
                        std::cout << "log config error: appender type is null, "
                                  << a << std::endl;
                        continue;
                    }
                    std::string type = a["type"].as<std::string>();
                    LogAppenderDefine lad;
                    if (type == "FileLogAppender") {
                        lad.type = 1;
                        if (!a["file"].IsDefined()) {
                            std::cout << "log config error: fileappender file "
                                         "is null, "
                                      << a << std::endl;
                            continue;
                        }
                        lad.file = a["file"].as<std::string>();
                        if (a["formatter"].IsDefined()) {
                            lad.formatter = a["formatter"].as<std::string>();
                        }
                    } else if (type == "StdoutLogAppender") {
                        lad.type = 2;
                    } else {
                        std::cout
                            << "log config error: appender type is invalid, "
                            << a << std::endl;
                        continue;
                    }
                    ld.appenders.push_back(lad);
                }
            }
            ld_set.insert(ld);
        }

        return ld_set;
    }
};

template <> class LexicalCast<std::set<LogDefine>, std::string> {
public:
    std::string operator()(const std::set<LogDefine> &v) {
        YAML::Node node;
        for (auto &i : v) {
            YAML::Node n;
            n["name"] = i.name;
            if (i.level != LogLevel::UNKNOWN) {
                n["level"] = LogLevel::ToString(i.level);
            }
            if (!i.formatter.empty()) {
                n["formatter"] = i.formatter;
            }

            for (auto &a : i.appenders) {
                YAML::Node na;
                if (a.type == 1) {
                    na["type"] = "FileLogAppender";
                    na["file"] = a.file;
                } else if (a.type == 2) {
                    na["type"] = "StdoutLogAppender";
                }
                if (a.level != LogLevel::UNKNOWN) {
                    na["level"] = LogLevel::ToString(a.level);
                }

                if (!a.formatter.empty()) {
                    na["formatter"] = a.formatter;
                }

                n["appenders"].push_back(na);
            }
            node.push_back(n);
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

sylar::ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
    sylar::ConfigManager::LookUp("logs", std::set<LogDefine>(), "logs config");

struct LogIniter {
    LogIniter() {
        g_log_defines->addListener(
            0x11223344, [](const std::set<LogDefine> &old_value,
                           const std::set<LogDefine> &new_value) {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "on_logger_conf_changed";
                for (auto &i : new_value) {
                    auto it = old_value.find(i);
                    sylar::Logger::ptr logger;
                    if (it == old_value.end()) {
                        //新增logger
                        logger = SYLAR_LOG_NAME(i.name);
                    } else {
                        if (i == *it) {
                            //修改的logger
                            logger = SYLAR_LOG_NAME(i.name);
                        }
                    }

                    logger->setLevel(i.level);
                    if (!i.formatter.empty()) {
                        logger->setFormatter(i.formatter);
                    }

                    logger->clearAppenders();
                    for (auto &a : i.appenders) {
                        sylar::LogAppender::ptr ap;

                        if (a.type == 1) {
                            ap.reset(new FileLogAppender(a.file));
                        } else if (a.type == 2) {
                            ap.reset(new StdoutLogAppender);
                        }

                        ap->setLevel(a.level);
                        if (!a.formatter.empty()) {
                            LogFormatter::ptr fmt(
                                new LogFormatter(a.formatter));
                            if (!fmt->isError()) {
                                ap->setFormater(fmt);
                            } else {
                                std::cout << "log.name=" << i.name
                                          << "appender type =" << a.type
                                          << "formatter=" << a.formatter
                                          << "is invalid" << std::endl;
                            }
                        }

                        logger->addAppender(ap);
                    }
                }

                for (auto &i : old_value) {
                    auto it = new_value.find(i);
                    if (it == new_value.end()) {
                        //删除logger
                        auto logger = SYLAR_LOG_NAME(i.name);
                        logger->setLevel(LogLevel::UNKNOWN);
                        logger->clearAppenders();
                    }
                }
            });
    }
};

//在main函数之前注册配置更改的回调函数
//用于在更新配置时将log相关的配置加载到LoggerManager
static LogIniter __log_init;

} // end namespace sylar
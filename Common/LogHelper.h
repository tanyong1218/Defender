#pragma once
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <boost/shared_array.hpp>
#include <boost/log/common.hpp>
//#include <mutex>

#ifdef _WIN32
#define __FILENAME__ (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1):__FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)
#endif

//定义一个在日志后添加 文件名 函数名 行号 的宏定义
#ifndef suffix
#define suffix(msg)  std::string("[") \
        .append(__FILENAME__).append("::").append(__func__) \
        .append("::").append(std::to_string(__LINE__)) \
        .append("]: ").append(msg)
#endif

template<typename... Args>
using format_string_t = fmt::format_string<Args...>;

class LogHelper
{
public:
    LogHelper(const LogHelper&) = delete;
    LogHelper& operator=(const LogHelper&) = delete;
    ~LogHelper();
    static LogHelper& GetInstance();
    std::shared_ptr<spdlog::logger> GetLogger();

public:
    template<typename... Args>
    inline void info(format_string_t<Args...> fmt, Args &&...args)
    {
        auto msg = fmt::format(fmt, std::forward<Args>(args)...);
        spdlog::info(("{}"), msg);
        GetLogger()->info(("{}"), std::move(msg));
    }
    template<typename... Args>
    inline void error(format_string_t<Args...> fmt, Args &&...args)
    {
        auto msg = fmt::format(fmt, std::forward<Args>(args)...);
        spdlog::error(("{}"), msg);
        GetLogger()->error(("{}"), std::move(msg));
    }
    template<typename... Args>
    inline void debug(format_string_t<Args...> fmt, Args &&...args)
    {
        auto msg = fmt::format(fmt, std::forward<Args>(args)...);
        spdlog::debug(("{}"), (msg));
        GetLogger()->debug(("{}"), std::move(msg));
    }
    template<typename... Args>
    inline void critical(format_string_t<Args...> fmt, Args &&...args)
    {
        auto msg = fmt::format(fmt, std::forward<Args>(args)...);
        spdlog::critical(("{}"), (msg));
        GetLogger()->critical(("{}"), std::move(msg));
    }
    template<typename... Args>
    inline void trace(format_string_t<Args...> fmt, Args &&...args)
    {
        auto msg = fmt::format(fmt, std::forward<Args>(args)...);
        spdlog::trace(("{}"), (msg));
        GetLogger()->trace(("{}"), std::move(msg));
    }
    template<typename... Args>
    inline void warn(format_string_t<Args...> fmt, Args &&...args)
    {
        auto msg = fmt::format(fmt, std::forward<Args>(args)...);
        spdlog::warn(("{}"), (msg));
        GetLogger()->warn(("{}"), std::move(msg));
    }

private:
    LogHelper();
    std::shared_ptr<spdlog::logger> m_logger;
};


#define WriteTrace(msg,...) LogHelper::GetInstance().trace(suffix(msg),__VA_ARGS__)
#define WriteDebug(msg,...) LogHelper::GetInstance().debug(suffix(msg),__VA_ARGS__)
#define WriteInfo(msg,...) LogHelper::GetInstance().info(suffix(msg),__VA_ARGS__)
#define WriteWarn(msg,...) LogHelper::GetInstance().warn(suffix(msg),__VA_ARGS__)
#define WriteError(msg,...) LogHelper::GetInstance().error(suffix(msg),__VA_ARGS__)
#define WriteCritical(msg,...) LogHelper::GetInstance().critical(suffix(msg),__VA_ARGS__)
#pragma once
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <boost/shared_array.hpp>
#include <boost/log/common.hpp>
#include <mutex>

#ifdef _WIN32
#define __FILENAME__ (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1):__FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)
#endif

//定义一个在日志后添加 文件名 函数名 行号 的宏定义
#ifndef suffix
#define suffix(msg)  std::string(msg).append("  <")\
        .append(__FILENAME__).append("> <").append(__func__)\
        .append("> <").append(std::to_string(__LINE__))\
        .append(">").c_str()
#endif

class LogHelper
{
public:
	~LogHelper();
	static LogHelper& GetInstance();
	std::shared_ptr<spdlog::logger> GetLogger();

public:
	LogHelper(const LogHelper&) = delete;
	LogHelper& operator=(const LogHelper&) = delete;

private:
	LogHelper();
	std::shared_ptr<spdlog::logger> m_logger;
};


#define WriteTrace(msg,...) LogHelper::GetInstance().GetLogger()->trace(suffix(msg),__VA_ARGS__)
#define WriteDebug(...) LogHelper::GetInstance().GetLogger()->debug(__VA_ARGS__)
#define WriteInfo(...) LogHelper::GetInstance().GetLogger()->info(__VA_ARGS__)
#define WriteWarn(...) LogHelper::GetInstance().GetLogger()->warn(__VA_ARGS__)
#define WriteError(msg,...) LogHelper::GetInstance().GetLogger()->error(suffix(msg),__VA_ARGS__)
#define WriteCritical(...) LogHelper::GetInstance().GetLogger()->critical(__VA_ARGS__)

#define criticalif(b, ...)                        \
    do {                                       \
        if ((b)) {                             \
           Logger::GetInstance().GetLogger()->critical(__VA_ARGS__); \
        }                                      \
    } while (0)
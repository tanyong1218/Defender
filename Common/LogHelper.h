#pragma once
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <boost/shared_array.hpp>
#include <boost/log/common.hpp>
#include <mutex>

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
#define WriteError(...) LogHelper::GetInstance().GetLogger()->error(__VA_ARGS__)
#define WriteCritical(...) LogHelper::GetInstance().GetLogger()->critical(__VA_ARGS__)

#define criticalif(b, ...)                        \
    do {                                       \
        if ((b)) {                             \
           Logger::GetInstance().GetLogger()->critical(__VA_ARGS__); \
        }                                      \
    } while (0)
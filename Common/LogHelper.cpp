#include "LogHelper.h"
#include "spdlog/async.h"

LogHelper::~LogHelper()
{
	spdlog::drop_all();
}

LogHelper::LogHelper()
{
	spdlog::init_thread_pool(10240, 2);
	std::vector<spdlog::sink_ptr> sinkList;
	auto basicSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("../logs/Main.txt");
	basicSink->set_level(spdlog::level::debug);
	basicSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%5l%$]  %v");
	sinkList.push_back(std::move(basicSink));
	// Consider using spdlog::async_logger for thread-safe logging
	m_logger = std::make_shared<spdlog::async_logger>("log", begin(sinkList), end(sinkList), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
	spdlog::register_logger(m_logger);
	m_logger->set_level(spdlog::level::err);
	m_logger->flush_on(spdlog::level::err);
	spdlog::flush_every(std::chrono::seconds(3));
}

LogHelper& LogHelper::GetInstance()
{
	static LogHelper m_instance;
	return m_instance;
}

std::shared_ptr<spdlog::logger> LogHelper::GetLogger()
{
	return m_logger;
}


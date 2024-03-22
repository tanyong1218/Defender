#include "pch.h"
#include "LogHelper.h"


LogHelper::~LogHelper()
{
	spdlog::drop_all();
}

LogHelper::LogHelper()
{
	std::vector<spdlog::sink_ptr> sinkList;
	auto basicSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("../logs/basicSink.txt");
	basicSink->set_level(spdlog::level::debug);
	basicSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%5l%$]  %v");
	sinkList.push_back(basicSink);
	m_logger = std::make_shared<spdlog::logger>("both", begin(sinkList), end(sinkList));
	//register it if you need to access it globally
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

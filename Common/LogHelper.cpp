#include "LogHelper.h"
#include "spdlog/async.h"
#include "spdlog/sinks/rotating_file_sink.h"

LogHelper::~LogHelper()
{
	spdlog::drop_all();
}

LogHelper::LogHelper()
{
	//mpmc_blocking_queue ��СΪ1024���̳߳��߳���Ϊ4
	spdlog::init_thread_pool(1024, 4);

	std::vector<spdlog::sink_ptr> sinkList;

	//��������Sink��ÿ���ļ���СΪ10M����ౣ��5���ļ�
	auto basicSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("../logs/Main.txt", 1024 * 1024 * 10, 5);
	basicSink->set_level(spdlog::level::debug);
	basicSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%5l%$] [Thread:%t] %v");

	//������ɫ�ն����sink
	auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	consoleSink->set_level(spdlog::level::debug);
	consoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%5l%$] [Thread:%t] %v");

	sinkList.push_back(std::move(basicSink));
	sinkList.push_back(std::move(consoleSink));

	m_logger = std::make_shared<spdlog::async_logger>("log", begin(sinkList), end(sinkList), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
	spdlog::register_logger(m_logger);
	m_logger->set_level(spdlog::level::debug);
	m_logger->flush_on(spdlog::level::err);
	spdlog::flush_every(std::chrono::seconds(3));
}

LogHelper& LogHelper::GetInstance()
{
	static LogHelper instance;
	return instance;
}

std::shared_ptr<spdlog::logger> LogHelper::GetLogger()
{
	return m_logger;
}


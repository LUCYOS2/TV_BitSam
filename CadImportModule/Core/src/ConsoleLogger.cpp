#include "Core/Logging/ConsoleLogger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

namespace CadImport::Core
{
    ConsoleLogger::ConsoleLogger(std::ostream& out) : m_out(out)
    {
    }

    ConsoleLogger::ConsoleLogger() : ConsoleLogger(std::cout)
    {
    }

    void ConsoleLogger::Debug(const std::string& message) { Write("DEBUG", message); }
    void ConsoleLogger::Info(const std::string& message) { Write("INFO", message); }
    void ConsoleLogger::Warn(const std::string& message) { Write("WARN", message); }
    void ConsoleLogger::Error(const std::string& message) { Write("ERROR", message); }

    void ConsoleLogger::Write(const char* level, const std::string& message)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        const auto now = std::chrono::system_clock::now();
        const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
        std::tm localTm{};
#if defined(_WIN32)
        localtime_s(&localTm, &nowTime);
#else
        localtime_r(&nowTime, &localTm);
#endif

        m_out << "[" << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S") << "] "
              << "[" << level << "] " << message << std::endl;
    }
}

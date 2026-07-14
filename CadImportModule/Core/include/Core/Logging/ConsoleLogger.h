#pragma once

#include <mutex>
#include <ostream>

#include "Core/Interfaces/ILogger.h"

namespace CadImport::Core
{
    // Thread-safe logger that writes timestamped, leveled lines to a
    // std::ostream (defaults to std::cout). Kept dependency-free so it can
    // be linked into Core without pulling in any NX headers.
    class ConsoleLogger : public ILogger
    {
    public:
        explicit ConsoleLogger(std::ostream& out);
        ConsoleLogger();

        void Debug(const std::string& message) override;
        void Info(const std::string& message) override;
        void Warn(const std::string& message) override;
        void Error(const std::string& message) override;

    private:
        void Write(const char* level, const std::string& message);

        std::ostream& m_out;
        std::mutex m_mutex;
    };
}

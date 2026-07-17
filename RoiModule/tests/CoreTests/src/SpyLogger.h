#pragma once

#include <string>
#include <vector>

#include "Core/Interfaces/ILogger.h"

namespace CadImport::Core::Tests
{
    // Captures every call instead of printing - lets tests assert on
    // exactly what ROIManager logged (and how many times) without
    // scraping stdout.
    class SpyLogger : public ILogger
    {
    public:
        void Debug(const std::string& message) override { debugMessages.push_back(message); }
        void Info(const std::string& message) override { infoMessages.push_back(message); }
        void Warn(const std::string& message) override { warnMessages.push_back(message); }
        void Error(const std::string& message) override { errorMessages.push_back(message); }

        std::vector<std::string> debugMessages;
        std::vector<std::string> infoMessages;
        std::vector<std::string> warnMessages;
        std::vector<std::string> errorMessages;
    };
}

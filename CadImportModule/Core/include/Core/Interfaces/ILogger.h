#pragma once

#include <string>

namespace CadImport::Core
{
    class ILogger
    {
    public:
        virtual ~ILogger() = default;

        virtual void Debug(const std::string& message) = 0;
        virtual void Info(const std::string& message) = 0;
        virtual void Warn(const std::string& message) = 0;
        virtual void Error(const std::string& message) = 0;
    };
}

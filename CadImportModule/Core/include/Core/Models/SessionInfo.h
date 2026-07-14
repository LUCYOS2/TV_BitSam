#pragma once

#include <string>

namespace CadImport::Core
{
    struct SessionInfo
    {
        std::string modelName;
        std::string revision;
        std::string partName;
        std::string unit;      // e.g. "mm"
        bool connected = false;
    };
}

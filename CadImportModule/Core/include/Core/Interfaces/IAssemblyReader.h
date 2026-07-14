#pragma once

#include "Core/Models/ComponentInfo.h"
#include "Core/Models/OperationResult.h"

namespace CadImport::Core
{
    // Reads the full Assembly Tree of the currently loaded Display Part.
    class IAssemblyReader
    {
    public:
        virtual ~IAssemblyReader() = default;

        // Returns the root ComponentInfo, with the full tree in ::children.
        virtual OperationResult<ComponentInfo> ReadTree() = 0;
    };
}

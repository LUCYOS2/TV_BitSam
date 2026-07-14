#pragma once

#include <string>
#include <vector>

#include "Core/Models/MaterialInfo.h"
#include "Core/Models/OperationResult.h"

namespace CadImport::Core
{
    // Reads Material *names* attached to a component's bodies. Mapping a
    // material name to optical properties (reflectance/transmittance/
    // absorptance) is the Material Engine module's responsibility, not
    // this one's.
    class IMaterialReader
    {
    public:
        virtual ~IMaterialReader() = default;

        virtual OperationResult<std::vector<MaterialInfo>> ReadMaterial(const std::string& componentName) = 0;
    };
}

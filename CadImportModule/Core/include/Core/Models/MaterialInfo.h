#pragma once

#include <string>

namespace CadImport::Core
{
    // Material *name* only - optical property mapping (reflectance/
    // transmittance/absorptance) belongs to the Material Engine module,
    // not this one.
    struct MaterialInfo
    {
        std::string componentName;
        std::string bodyId;        // optional, empty if material is component-level
        std::string materialName;
    };
}

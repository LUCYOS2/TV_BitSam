#pragma once

#include <string>
#include <vector>

namespace CadImport::Core
{
    // Mirrors an NX Assembly component node. Tree is built by AssemblyReader
    // by recursively walking ComponentAssembly::RootComponent.
    struct ComponentInfo
    {
        std::string name;
        std::string partName;
        std::string parentName;    // empty for root
        bool visible = true;
        bool suppressed = false;
        std::vector<ComponentInfo> children;
    };
}

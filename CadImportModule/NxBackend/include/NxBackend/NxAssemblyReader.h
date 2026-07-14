#pragma once

#include "Core/Interfaces/IAssemblyReader.h"
#include "Core/Interfaces/ILogger.h"
#include "NxBackend/NxConnector.h"

namespace NXOpen::Assemblies
{
    class Component;
}

namespace CadImport::NxBackend
{
    // Recursively walks ComponentAssembly::RootComponent to build the
    // Component tree. Depends only on an already-connected NxConnector -
    // does not manage session lifetime itself (SRP).
    class NxAssemblyReader : public Core::IAssemblyReader
    {
    public:
        NxAssemblyReader(NxConnector* connector, Core::ILogger* logger);

        Core::OperationResult<Core::ComponentInfo> ReadTree() override;

    private:
        static Core::ComponentInfo BuildComponentInfo(NXOpen::Assemblies::Component* component, const std::string& parentName);

        NxConnector* m_connector;  // not owned
        Core::ILogger* m_logger;   // not owned
    };
}

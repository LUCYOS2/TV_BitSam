#pragma once

#include <vector>

#include "Core/Interfaces/IMaterialReader.h"
#include "Core/Interfaces/ILogger.h"
#include "NxBackend/NxConnector.h"

namespace CadImport::NxBackend
{
    // Reads the Material *name* attached to each body of a component.
    // Does NOT map that name to optical properties (reflectance/
    // transmittance/absorptance) - that mapping belongs to the Material
    // Engine module.
    class NxMaterialReader : public Core::IMaterialReader
    {
    public:
        NxMaterialReader(NxConnector* connector, Core::ILogger* logger);

        Core::OperationResult<std::vector<Core::MaterialInfo>> ReadMaterial(const std::string& componentName) override;

    private:
        NxConnector* m_connector;  // not owned
        Core::ILogger* m_logger;   // not owned
    };
}

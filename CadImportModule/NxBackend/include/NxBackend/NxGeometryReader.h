#pragma once

#include "Core/Interfaces/IGeometryReader.h"
#include "Core/Interfaces/ILogger.h"
#include "NxBackend/NxConnector.h"

namespace NXOpen
{
    class Body;
}

namespace CadImport::NxBackend
{
    // Reads summary-level geometry (Body/Face/Edge counts + BoundingBox)
    // for a named component. Display-only in this phase - no tessellation.
    class NxGeometryReader : public Core::IGeometryReader
    {
    public:
        NxGeometryReader(NxConnector* connector, Core::ILogger* logger);

        Core::OperationResult<Core::GeometryInfo> ReadGeometry(const std::string& componentName) override;

    private:
        static Core::BoundingBox3D ComputeBoundingBox(NXOpen::Body* body);

        NxConnector* m_connector;  // not owned
        Core::ILogger* m_logger;   // not owned
    };
}

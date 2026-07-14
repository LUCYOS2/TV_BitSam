#pragma once

#include <string>

#include "Core/Models/GeometryInfo.h"
#include "Core/Models/OperationResult.h"

namespace CadImport::Core
{
    // Reads summary-level geometry (Body/Face/Edge counts, BoundingBox) for
    // a single component. Full tessellation/meshing is out of scope here -
    // that belongs to the future ROI/mesh module.
    class IGeometryReader
    {
    public:
        virtual ~IGeometryReader() = default;

        virtual OperationResult<GeometryInfo> ReadGeometry(const std::string& componentName) = 0;
    };
}

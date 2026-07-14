#pragma once

#include <string>
#include <vector>

#include "Core/Models/BoundingBox3D.h"

namespace CadImport::Core
{
    // Summary-level geometry data for a single Body.
    // Display-only in this phase - no mesh/tessellation data yet.
    struct BodyInfo
    {
        std::string bodyId;    // NX journal identifier / tag string
        int faceCount = 0;
        int edgeCount = 0;
        BoundingBox3D boundingBox;
    };

    struct GeometryInfo
    {
        std::string componentName;
        std::vector<BodyInfo> bodies;
    };
}

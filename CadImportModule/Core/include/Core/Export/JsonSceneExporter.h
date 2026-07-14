#pragma once

#include <string>
#include <vector>

#include "Core/Models/ComponentInfo.h"
#include "Core/Models/GeometryInfo.h"
#include "Core/Models/MaterialInfo.h"
#include "Core/Models/OperationResult.h"
#include "Core/Models/ROISelection.h"
#include "Core/Models/SessionInfo.h"

namespace CadImport::Core
{
    // Serializes this module's output into a single JSON "Scene Manifest"
    // file - the proposed hand-off interface to the Material Engine and
    // Ray Tracing modules (see docs/DevGuide.md for the schema).
    class JsonSceneExporter
    {
    public:
        std::string ToJson(
            const SessionInfo& sessionInfo,
            const ComponentInfo& rootComponent,
            const std::vector<GeometryInfo>& geometry,
            const std::vector<MaterialInfo>& materials,
            const std::vector<ROISelection>& roiSelections) const;

        // Convenience overload: builds the JSON and writes it to filePath.
        OperationResult<bool> WriteToFile(
            const std::string& filePath,
            const SessionInfo& sessionInfo,
            const ComponentInfo& rootComponent,
            const std::vector<GeometryInfo>& geometry,
            const std::vector<MaterialInfo>& materials,
            const std::vector<ROISelection>& roiSelections) const;

    private:
        static void WriteComponent(class JsonWriter& writer, const ComponentInfo& component);
    };
}

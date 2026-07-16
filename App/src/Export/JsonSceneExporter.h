#pragma once

#include <string>
#include <vector>

#include "Core/Models/ComponentInfo.h"
#include "Core/Models/GeometryInfo.h"
#include "Core/Models/MaterialInfo.h"
#include "Core/Models/OperationResult.h"
#include "Core/Models/ROISelection.h"
#include "Core/Models/SessionInfo.h"

namespace CadImport::App
{
    // Combines CadImportModule's output (SessionInfo/ComponentInfo/
    // GeometryInfo/MaterialInfo) and RoiModule's output (ROISelection) into
    // a single JSON "Scene Manifest" - the proposed hand-off interface to
    // the Material Engine and Ray Tracing modules (see
    // CadImportModule/docs/DevGuide.md for the schema).
    //
    // Lives in App (not either module's Core) because combining both
    // modules' types is an integration-level concern - putting it in either
    // module's Core would make that module depend on the other's Core,
    // defeating the point of splitting them.
    class JsonSceneExporter
    {
    public:
        std::string ToJson(
            const Core::SessionInfo& sessionInfo,
            const Core::ComponentInfo& rootComponent,
            const std::vector<Core::GeometryInfo>& geometry,
            const std::vector<Core::MaterialInfo>& materials,
            const std::vector<Core::ROISelection>& roiSelections) const;

        // Convenience overload: builds the JSON and writes it to filePath.
        Core::OperationResult<bool> WriteToFile(
            const std::string& filePath,
            const Core::SessionInfo& sessionInfo,
            const Core::ComponentInfo& rootComponent,
            const std::vector<Core::GeometryInfo>& geometry,
            const std::vector<Core::MaterialInfo>& materials,
            const std::vector<Core::ROISelection>& roiSelections) const;

    private:
        static void WriteComponent(class JsonWriter& writer, const Core::ComponentInfo& component);
    };
}

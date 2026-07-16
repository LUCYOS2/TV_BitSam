#pragma once

#include <string>

#include "Core/Models/OperationResult.h"
#include "Core/Models/ROISelection.h"

namespace CadImport::Core
{
    // Interactively resolves an ROI pick into real geometry data - no JT
    // file, no separate mesh export. The concrete (NxBackend) implementation
    // prompts the user to click a Face in the NX graphics window and reads
    // component/body/area/boundingBox directly from the live NX session.
    //
    // This is a *blocking* call: it does not return until the user picks a
    // Face or cancels. Keep this in mind at call sites that expect a fully
    // unattended/batch pipeline (see App/src/main.cpp --pick-roi handling).
    class IRoiResolver
    {
    public:
        virtual ~IRoiResolver() = default;

        virtual OperationResult<ROISelection> PromptSelectFace(const std::string& note = "") = 0;
    };
}

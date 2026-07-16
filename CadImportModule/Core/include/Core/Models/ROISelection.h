#pragma once

#include <string>

#include "Core/Models/BoundingBox3D.h"

namespace CadImport::Core
{
    enum class ROISelectionKind
    {
        Point,
        Face,
        Component
    };

    // Record of a user selection. Selections made through IROIManager's
    // SelectPoint/SelectFace/SelectComponent are unresolved (id/note only) -
    // no geometric data attached. Selections resolved through
    // IRoiResolver::PromptSelectFace are geometry-backed: the user picked a
    // real Face in the NX graphics window, and componentName/bodyId/area/
    // boundingBox are filled in directly from the live NX session (no JT
    // file involved).
    struct ROISelection
    {
        ROISelectionKind kind = ROISelectionKind::Component;
        std::string targetId;      // NX journal identifier of the picked entity
        std::string note;

        bool resolved = false;
        std::string componentName;
        std::string bodyId;
        double area = 0.0;         // mm^2, only meaningful when kind == Face
        BoundingBox3D boundingBox;
    };
}

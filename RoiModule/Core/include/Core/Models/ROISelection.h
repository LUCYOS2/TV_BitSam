#pragma once

#include <string>

#include "Core/Models/BoundingBox3D.h"
#include "Core/Models/FacetMesh.h"

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
    // no geometric data attached. Selections resolved through one of
    // IRoiResolver's Prompt*/Resolve* methods are geometry-backed:
    // componentName/bodyId/boundingBox (and area/coordinate/mesh where
    // applicable - see field comments below) are filled in directly from
    // the live NX session, no JT file involved. Component-kind selections
    // resolve via IRoiResolver::PromptSelectBodiesInBox (a whole Body, not
    // a single Face) - typically the building block of a local ROI scope
    // (see App/src/main.cpp --roi-scope handling).
    struct ROISelection
    {
        ROISelectionKind kind = ROISelectionKind::Component;
        std::string targetId;      // NX journal identifier of the picked entity
        std::string note;
        std::string scopeId;       // groups selections from one local ROI
                                    // pass (e.g. "bottom-corner", "back-center") -
                                    // set by the App orchestration layer, not by
                                    // IRoiResolver; empty means "no scope"

        bool resolved = false;
        std::string componentName;
        std::string bodyId;
        double area = 0.0;         // mm^2, only meaningful when kind == Face
        BoundingBox3D boundingBox;
        Point3D coordinate;        // only meaningful when kind == Point (see
                                    // IRoiResolver::ResolvePointAtCoordinate) -
                                    // the picked/authored coordinate itself,
                                    // as opposed to boundingBox which describes
                                    // whatever body/face it resolved against
        FacetMesh mesh;            // empty/valid=false unless explicitly
                                    // populated via IRoiResolver::ExtractFacetMesh -
                                    // not computed on every pick (see FacetMesh.h)
    };
}

#pragma once

#include <vector>

#include "Core/Interfaces/IRoiResolver.h"
#include "Core/Interfaces/ILogger.h"
#include "NxContracts/INxSessionAccessor.h"

namespace NXOpen
{
    class Face;
    class Body;
}

namespace CadImport::NxBackend
{
    // Resolves an ROI pick into real geometry data straight from the live
    // NX session - no JT file, no separate mesh export (see plan doc /
    // CadImportModule/docs/DevGuide.md "Interactive ROI Picking" section
    // for why).
    //
    // PromptSelectFace() blocks until the user clicks a Face in the NX
    // graphics window (or cancels) via NXOpen::UI's SelectionManager.
    //
    // Depends only on NxContracts::INxSessionAccessor (not CadImportModule's
    // concrete NxConnector) - this is the module boundary between RoiModule
    // and CadImportModule: RoiModule never needs to know how the session was
    // acquired, only that it can ask for the raw NXOpen::Session*.
    class NxRoiResolver : public Core::IRoiResolver
    {
    public:
        NxRoiResolver(NxContracts::INxSessionAccessor* sessionAccessor, Core::ILogger* logger);

        Core::OperationResult<Core::ROISelection> PromptSelectFace(const std::string& note = "") override;
        Core::OperationResult<std::vector<Core::ROISelection>> PromptSelectFacesInBox(const std::string& note = "") override;
        Core::OperationResult<std::vector<Core::ROISelection>> PromptSelectBodiesInBox(const std::string& note = "") override;
        Core::OperationResult<Core::ROISelection> ResolvePointAtCoordinate(const Core::Point3D& coordinate, const std::string& note = "") override;
        Core::OperationResult<Core::FacetMesh> ExtractFacetMesh(const Core::ROISelection& selection) override;

    private:
        // Not static (despite taking all their "real" data as parameters):
        // both log through m_logger, which is an instance member.
        Core::OperationResult<Core::ROISelection> BuildSelectionFromFace(NXOpen::Face* face, const std::string& note);
        Core::OperationResult<Core::ROISelection> BuildSelectionFromBody(NXOpen::Body* body, const Core::Point3D& coordinate, const std::string& note);

        // Component-kind counterpart of BuildSelectionFromBody: no
        // coordinate (there wasn't one - the whole Body came from a box
        // pick, not a point resolve), otherwise the same fields.
        Core::OperationResult<Core::ROISelection> BuildSelectionFromBodyAsComponent(NXOpen::Body* body, const std::string& note);

        // Returns 0.0 (and logs a Warn, not an Error - the rest of the
        // selection is still valid/useful) if mass-properties extraction
        // itself fails for this face, e.g. a degenerate/zero-area face.
        double ComputeFaceArea(NXOpen::Face* face);

        // Triangulates a single Face. Shared by ExtractFacetMesh's
        // Face-kind path (one call) and its Point-kind path (called once
        // per face of the containing body, then merged).
        Core::OperationResult<Core::FacetMesh> FacetFace(NXOpen::Face* face);

        NxContracts::INxSessionAccessor* m_sessionAccessor;  // not owned
        Core::ILogger* m_logger;                             // not owned
    };
}

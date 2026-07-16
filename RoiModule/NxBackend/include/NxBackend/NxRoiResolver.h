#pragma once

#include "Core/Interfaces/IRoiResolver.h"
#include "Core/Interfaces/ILogger.h"
#include "NxContracts/INxSessionAccessor.h"

namespace NXOpen
{
    class Face;
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

    private:
        static Core::OperationResult<Core::ROISelection> BuildSelectionFromFace(NXOpen::Face* face, const std::string& note);

        NxContracts::INxSessionAccessor* m_sessionAccessor;  // not owned
        Core::ILogger* m_logger;                             // not owned
    };
}

#pragma once

#include "Core/Interfaces/IRoiResolver.h"
#include "Core/Interfaces/ILogger.h"
#include "NxBackend/NxConnector.h"

namespace NXOpen
{
    class Face;
}

namespace CadImport::NxBackend
{
    // Resolves an ROI pick into real geometry data straight from the live
    // NX session - no JT file, no separate mesh export (see plan doc /
    // docs/DevGuide.md "Interactive ROI Picking" section for why).
    //
    // PromptSelectFace() blocks until the user clicks a Face in the NX
    // graphics window (or cancels) via NXOpen::UI's SelectionManager.
    class NxRoiResolver : public Core::IRoiResolver
    {
    public:
        NxRoiResolver(NxConnector* connector, Core::ILogger* logger);

        Core::OperationResult<Core::ROISelection> PromptSelectFace(const std::string& note = "") override;

    private:
        static Core::OperationResult<Core::ROISelection> BuildSelectionFromFace(NXOpen::Face* face, const std::string& note);

        NxConnector* m_connector;  // not owned
        Core::ILogger* m_logger;   // not owned
    };
}

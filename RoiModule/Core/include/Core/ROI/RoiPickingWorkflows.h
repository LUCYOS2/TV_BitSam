#pragma once

#include <string>

#include "Core/Interfaces/ILogger.h"
#include "Core/Interfaces/IROIManager.h"
#include "Core/Interfaces/IRoiResolver.h"
#include "Core/Models/ROISelection.h"

namespace CadImport::Core
{
    // One "pick attempt" each, factored out of App/src/main.cpp so they can
    // be exercised without a live NX session (see RoiModule/tests/CoreTests
    // MockRoiResolver-based coverage) - the console y/n repeat loop and
    // std::cin/cout around these stay in App, since that's an
    // interactive-CLI concern, not ROI logic. These functions only depend
    // on IRoiResolver/IROIManager (interfaces), never the concrete
    // NX-backed classes.
    //
    // Every pick function takes `attachMesh`: when true, IRoiResolver::
    // ExtractFacetMesh is called for each resolved selection before it's
    // stored (see TryAttachFacetMesh) - kept as a parameter rather than a
    // separate pass so callers never have to remember to attach it
    // themselves, but still opt-in since faceting is expensive.

    // Single Face pick. Returns false (having logged a Warn through
    // `logger` if non-null) if the resolver call itself failed or was
    // cancelled; true if a selection was recorded.
    bool TryAddFaceSelection(IRoiResolver& resolver, IROIManager& roiManager, const std::string& note, bool attachMesh, ILogger* logger);

    // Box/rubber-band pick - may resolve zero, one, or many Faces in a
    // single call. Returns how many selections were actually added (0 if
    // the pick failed/was cancelled or nothing resolved).
    int TryAddBoxSelections(IRoiResolver& resolver, IROIManager& roiManager, const std::string& note, bool attachMesh, ILogger* logger);

    // Box/rubber-band pick at whole-Body granularity, tagging every
    // resulting selection with `scopeId` - this is the building block of a
    // local ROI scope (see App/src/main.cpp --roi-scope handling): each
    // scope is one independent call to this function under a different
    // label, not a running/cumulative selection. Returns how many
    // selections were actually added.
    int TryAddBodySelectionsInBox(IRoiResolver& resolver, IROIManager& roiManager, const std::string& scopeId, const std::string& note, bool attachMesh, ILogger* logger);

    // Coordinate resolve - never blocks (see IRoiResolver::ResolvePointAtCoordinate).
    bool TryAddPointSelection(IRoiResolver& resolver, IROIManager& roiManager, const Point3D& coordinate, const std::string& note, bool attachMesh, ILogger* logger);

    // Extracts a facet mesh for an already-resolved selection and writes it
    // into selection.mesh in place. Returns false (logging a Warn) if
    // extraction failed - every other field of `selection` is left as-is
    // either way, so callers can always proceed to store it.
    bool TryAttachFacetMesh(IRoiResolver& resolver, ROISelection& selection, ILogger* logger);
}

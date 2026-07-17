#pragma once

#include <string>
#include <vector>

#include "Core/Models/OperationResult.h"
#include "Core/Models/ROISelection.h"

namespace CadImport::Core
{
    // Resolves an ROI pick into real geometry data - no JT file, no
    // separate mesh export. The concrete (NxBackend) implementation reads
    // component/body/area/boundingBox directly from the live NX session.
    //
    // Pick modes, added incrementally (OCP: each is a new method,
    // PromptSelectFace never changed shape when the others were added):
    //   - PromptSelectFace:        single Face, interactive click
    //   - PromptSelectFacesInBox:  multiple Faces, interactive box/rubber-
    //                              band drag (one prompt, many results)
    //   - PromptSelectBodiesInBox: multiple whole Bodies, same box/rubber-
    //                              band drag but Body-granularity - this is
    //                              what actually resolves ROISelectionKind::
    //                              Component (see ROISelection.h)
    //   - ResolvePointAtCoordinate: single Point, given directly by the
    //                              caller (config file, computed value, ...) -
    //                              not interactive, so it does not block
    //                              waiting on the NX graphics window
    class IRoiResolver
    {
    public:
        virtual ~IRoiResolver() = default;

        // *Blocking*: does not return until the user picks a Face or
        // cancels. Keep this in mind at call sites that expect a fully
        // unattended/batch pipeline (see App/src/main.cpp --pick-roi
        // handling).
        virtual OperationResult<ROISelection> PromptSelectFace(const std::string& note = "") = 0;

        // *Blocking*: does not return until the user drags a selection box
        // in the NX graphics window (or cancels). Every Face inside the box
        // comes back resolved in one call - see App/src/main.cpp
        // --pick-roi-box handling.
        virtual OperationResult<std::vector<ROISelection>> PromptSelectFacesInBox(const std::string& note = "") = 0;

        // *Blocking*: same box/rubber-band drag as PromptSelectFacesInBox,
        // but at whole-Body granularity - this is the mechanism behind a
        // "local ROI scope" (see App/src/main.cpp --roi-scope handling):
        // drag a box in a Front/Back view over the region you care about,
        // and every Body whose on-screen extent falls inside/crosses the
        // box comes back resolved as ROISelectionKind::Component. NX's own
        // SelectionManager does the screen-space hit-testing (same as
        // PromptSelectFacesInBox), so depth/view-angle is never our concern.
        virtual OperationResult<std::vector<ROISelection>> PromptSelectBodiesInBox(const std::string& note = "") = 0;

        // *Non-blocking*: the coordinate is already known (e.g. from a CLI
        // arg or a config file), so there is nothing to wait on. Resolves
        // to whichever body/component contains (or, failing that, is
        // nearest to) the coordinate in the live NX session - see
        // App/src/main.cpp --roi-point handling.
        virtual OperationResult<ROISelection> ResolvePointAtCoordinate(const Point3D& coordinate, const std::string& note = "") = 0;

        // Triangulates the geometry an already-resolved selection points
        // at (its Face, or every Face of its Body for a Point/Component
        // selection) into a FacetMesh - deliberately a separate call from
        // the pick methods above, not a field they fill in automatically,
        // since faceting is much more expensive than reading a bounding
        // box/area and callers that only need summary metadata shouldn't
        // pay for it. Requires selection.resolved == true and a non-empty
        // bodyId; fails clearly otherwise.
        virtual OperationResult<FacetMesh> ExtractFacetMesh(const ROISelection& selection) = 0;
    };
}

#pragma once

#include <string>

namespace CadImport::Core
{
    enum class ROISelectionKind
    {
        Point,
        Face,
        Component
    };

    // Placeholder record of a user selection. No geometric resolution
    // (meshing, area calculation, etc.) happens at this stage - that is
    // deferred to the future ROI computation module. This struct only
    // captures *what the user pointed at* so the ROI module has a
    // stable interface to build on.
    struct ROISelection
    {
        ROISelectionKind kind = ROISelectionKind::Component;
        std::string targetId;      // NX journal identifier of the picked entity
        std::string note;
    };
}

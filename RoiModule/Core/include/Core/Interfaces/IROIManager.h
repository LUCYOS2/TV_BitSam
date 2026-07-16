#pragma once

#include <string>
#include <vector>

#include "Core/Models/ROISelection.h"

namespace CadImport::Core
{
    // Placeholder interface for ROI selection. This phase only *records*
    // what the user picked (Point/Face/Component identifiers) - it does not
    // resolve those picks into geometry, mesh them, or compute area/leakage.
    // The real ROI computation module builds on top of this interface.
    class IROIManager
    {
    public:
        virtual ~IROIManager() = default;

        virtual void SelectPoint(const std::string& targetId, const std::string& note = "") = 0;
        virtual void SelectFace(const std::string& targetId, const std::string& note = "") = 0;
        virtual void SelectComponent(const std::string& targetId, const std::string& note = "") = 0;

        // Stores a selection that has already been fully resolved (e.g. by
        // IRoiResolver::PromptSelectFace) - geometry fields are kept as-is,
        // not recomputed.
        virtual void AddResolvedSelection(const ROISelection& selection) = 0;

        virtual void ClearSelections() = 0;
        virtual const std::vector<ROISelection>& GetSelections() const = 0;
    };
}

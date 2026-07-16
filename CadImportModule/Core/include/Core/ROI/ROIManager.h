#pragma once

#include <vector>

#include "Core/Interfaces/IROIManager.h"
#include "Core/Interfaces/ILogger.h"

namespace CadImport::Core
{
    // In-memory implementation of IROIManager. Stores selections in
    // insertion order; performs no geometric computation itself - resolved
    // selections (with geometry already filled in) come from an
    // IRoiResolver and are stored as-is via AddResolvedSelection.
    class ROIManager : public IROIManager
    {
    public:
        explicit ROIManager(ILogger* logger = nullptr);

        void SelectPoint(const std::string& targetId, const std::string& note = "") override;
        void SelectFace(const std::string& targetId, const std::string& note = "") override;
        void SelectComponent(const std::string& targetId, const std::string& note = "") override;
        void AddResolvedSelection(const ROISelection& selection) override;

        void ClearSelections() override;
        const std::vector<ROISelection>& GetSelections() const override;

    private:
        void AddSelection(ROISelectionKind kind, const std::string& targetId, const std::string& note);

        std::vector<ROISelection> m_selections;
        ILogger* m_logger;     // not owned, may be null
    };
}

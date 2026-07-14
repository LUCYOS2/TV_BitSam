#include "Core/ROI/ROIManager.h"

namespace CadImport::Core
{
    ROIManager::ROIManager(ILogger* logger) : m_logger(logger)
    {
    }

    void ROIManager::SelectPoint(const std::string& targetId, const std::string& note)
    {
        AddSelection(ROISelectionKind::Point, targetId, note);
    }

    void ROIManager::SelectFace(const std::string& targetId, const std::string& note)
    {
        AddSelection(ROISelectionKind::Face, targetId, note);
    }

    void ROIManager::SelectComponent(const std::string& targetId, const std::string& note)
    {
        AddSelection(ROISelectionKind::Component, targetId, note);
    }

    void ROIManager::ClearSelections()
    {
        m_selections.clear();
        if (m_logger)
        {
            m_logger->Debug("ROIManager: selections cleared");
        }
    }

    const std::vector<ROISelection>& ROIManager::GetSelections() const
    {
        return m_selections;
    }

    void ROIManager::AddSelection(ROISelectionKind kind, const std::string& targetId, const std::string& note)
    {
        ROISelection selection;
        selection.kind = kind;
        selection.targetId = targetId;
        selection.note = note;
        m_selections.push_back(std::move(selection));

        if (m_logger)
        {
            m_logger->Debug("ROIManager: recorded selection targetId=" + targetId);
        }
    }
}

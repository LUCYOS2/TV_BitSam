#include "Core/ROI/RoiPickingWorkflows.h"

namespace CadImport::Core
{
    bool TryAddFaceSelection(IRoiResolver& resolver, IROIManager& roiManager, const std::string& note, bool attachMesh, ILogger* logger)
    {
        OperationResult<ROISelection> result = resolver.PromptSelectFace(note);
        if (!result.success)
        {
            if (logger) logger->Warn("ROI face pick did not complete: " + result.errorMessage);
            return false;
        }

        if (attachMesh)
        {
            TryAttachFacetMesh(resolver, result.value, logger);
        }
        roiManager.AddResolvedSelection(result.value);
        return true;
    }

    int TryAddBoxSelections(IRoiResolver& resolver, IROIManager& roiManager, const std::string& note, bool attachMesh, ILogger* logger)
    {
        OperationResult<std::vector<ROISelection>> result = resolver.PromptSelectFacesInBox(note);
        if (!result.success)
        {
            if (logger) logger->Warn("ROI box pick did not complete: " + result.errorMessage);
            return 0;
        }

        for (ROISelection& selection : result.value)
        {
            if (attachMesh)
            {
                TryAttachFacetMesh(resolver, selection, logger);
            }
            roiManager.AddResolvedSelection(selection);
        }
        return static_cast<int>(result.value.size());
    }

    int TryAddBodySelectionsInBox(IRoiResolver& resolver, IROIManager& roiManager, const std::string& scopeId, const std::string& note, bool attachMesh, ILogger* logger)
    {
        OperationResult<std::vector<ROISelection>> result = resolver.PromptSelectBodiesInBox(note);
        if (!result.success)
        {
            if (logger) logger->Warn("ROI body-box pick did not complete: " + result.errorMessage);
            return 0;
        }

        for (ROISelection& selection : result.value)
        {
            selection.scopeId = scopeId;
            if (attachMesh)
            {
                TryAttachFacetMesh(resolver, selection, logger);
            }
            roiManager.AddResolvedSelection(selection);
        }
        return static_cast<int>(result.value.size());
    }

    bool TryAddPointSelection(IRoiResolver& resolver, IROIManager& roiManager, const Point3D& coordinate, const std::string& note, bool attachMesh, ILogger* logger)
    {
        OperationResult<ROISelection> result = resolver.ResolvePointAtCoordinate(coordinate, note);
        if (!result.success)
        {
            if (logger) logger->Warn("ROI coordinate resolve failed: " + result.errorMessage);
            return false;
        }

        if (attachMesh)
        {
            TryAttachFacetMesh(resolver, result.value, logger);
        }
        roiManager.AddResolvedSelection(result.value);
        return true;
    }

    bool TryAttachFacetMesh(IRoiResolver& resolver, ROISelection& selection, ILogger* logger)
    {
        OperationResult<FacetMesh> meshResult = resolver.ExtractFacetMesh(selection);
        if (!meshResult.success)
        {
            if (logger)
            {
                logger->Warn("ROI facet mesh extraction failed for targetId=" + selection.targetId +
                              ": " + meshResult.errorMessage);
            }
            return false;
        }

        selection.mesh = meshResult.value;
        return true;
    }
}

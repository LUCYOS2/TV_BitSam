#include "NxBackend/NxRoiResolver.h"

#include <string>
#include <vector>

#include <NXOpen/Session.hxx>
#include <NXOpen/UI.hxx>
#include <NXOpen/Selection.hxx>
#include <NXOpen/Face.hxx>
#include <NXOpen/Body.hxx>
#include <NXOpen/TaggedObject.hxx>
#include <NXOpen/NXException.hxx>
#include <NXOpen/UF/UFSession.hxx>
#include <NXOpen/Assemblies/Component.hxx>

#include "NxContracts/NxGeometryUtils.h"

using namespace CadImport::Core;

namespace CadImport::NxBackend
{
    NxRoiResolver::NxRoiResolver(NxContracts::INxSessionAccessor* sessionAccessor, Core::ILogger* logger)
        : m_sessionAccessor(sessionAccessor), m_logger(logger)
    {
    }

    OperationResult<ROISelection> NxRoiResolver::PromptSelectFace(const std::string& note)
    {
        if (m_sessionAccessor == nullptr || !m_sessionAccessor->IsAvailable())
        {
            return OperationResult<ROISelection>::Fail("NX session is not available - connect via CadImportModule first");
        }

        try
        {
            // TODO(office-PC verify): NXOpen::UI::GetUI() is the standard
            // singleton accessor (parallel to Session::GetSession()), and
            // Selection is reached via ui->SelectionManager() - confirm the
            // exact accessor name/return type against NX2406's UI.hxx.
            NXOpen::UI* ui = NXOpen::UI::GetUI();
            NXOpen::Selection* selectionManager = ui->SelectionManager();

            // Face-only Mask Triple. TODO(office-PC verify): exact field
            // names/values for NXOpen::Selection::MaskTriple - this mirrors
            // the commonly documented pattern (Type=UF_solid_type,
            // SolidBodySubtype=UF_UI_SEL_FEATURE_ANY_FACE) but the office PC
            // headers are authoritative.
            NXOpen::Selection::MaskTriple faceMask;
            faceMask.Type = UF_solid_type;
            faceMask.Subtype = 0;
            faceMask.SolidBodySubtype = UF_UI_SEL_FEATURE_ANY_FACE;
            std::vector<NXOpen::Selection::MaskTriple> maskTriples{faceMask};

            NXOpen::TaggedObject* selectedObject = nullptr;
            NXOpen::Point3d cursorPosition;

            // TODO(office-PC verify): exact overload/parameter order/count
            // for SelectTaggedObject on NX2406 - SelectObject/SelectObjects
            // were deprecated in NX8 in favor of this method, but its
            // signature has some variation across NX versions.
            NXOpen::Selection::Response response = selectionManager->SelectTaggedObject(
                "Select ROI Face",
                "Select a Face for ROI",
                NXOpen::Selection::SelectionScopeWorkPart,
                NXOpen::Selection::SelectionActionClearAndEnableSpecific,
                false,
                false,
                maskTriples,
                &selectedObject,
                &cursorPosition);

            const bool userCancelled =
                (response == NXOpen::Selection::ResponseCancel) ||
                (response == NXOpen::Selection::ResponseBack) ||
                (selectedObject == nullptr);

            if (userCancelled)
            {
                if (m_logger) m_logger->Info("ROI face pick cancelled by user");
                return OperationResult<ROISelection>::Fail("User cancelled ROI face selection");
            }

            NXOpen::Face* face = dynamic_cast<NXOpen::Face*>(selectedObject);
            if (face == nullptr)
            {
                return OperationResult<ROISelection>::Fail("Selected object is not a Face");
            }

            return BuildSelectionFromFace(face, note);
        }
        catch (const NXOpen::NXException& ex)
        {
            const std::string msg = std::string("NXException while prompting for ROI face: ") + ex.Message();
            if (m_logger) m_logger->Error(msg);
            return OperationResult<ROISelection>::Fail(msg);
        }
    }

    OperationResult<ROISelection> NxRoiResolver::BuildSelectionFromFace(NXOpen::Face* face, const std::string& note)
    {
        ROISelection selection;
        selection.kind = ROISelectionKind::Face;
        selection.note = note;
        selection.resolved = true;

        try
        {
            selection.targetId = face->JournalIdentifier();

            NXOpen::Body* body = face->GetBody();
            if (body != nullptr)
            {
                selection.bodyId = body->JournalIdentifier();

                NXOpen::Assemblies::Component* owner = body->OwningComponent();
                selection.componentName = (owner != nullptr) ? owner->Name() : std::string();
            }

            selection.boundingBox = NxContracts::ComputeBoundingBoxForTag(face);

            // TODO(office-PC verify): Face area extraction - high-level
            // NXOpen doesn't expose area directly on Face; the documented
            // approach is the low-level UF mass-properties call
            // (UF_MODL_ask_mass_props_3d / "AskMassProps3d") on the face's
            // tag. Confirm the exact signature (density/accuracy params,
            // output array layout) against NX2406 and wire in the real
            // surface-area value here.
            selection.area = 0.0;

            if (m_logger)
            {
                m_logger->Info("ROI face resolved: targetId=" + selection.targetId +
                                " component=" + selection.componentName);
            }

            return OperationResult<ROISelection>::Ok(selection);
        }
        catch (const NXOpen::NXException& ex)
        {
            const std::string msg = std::string("NXException while resolving ROI face geometry: ") + ex.Message();
            if (m_logger) m_logger->Error(msg);
            return OperationResult<ROISelection>::Fail(msg);
        }
    }
}

#include "NxBackend/NxRoiResolver.h"

#include <string>
#include <vector>

#include <NXOpen/Session.hxx>
#include <NXOpen/UI.hxx>
#include <NXOpen/Selection.hxx>
#include <NXOpen/Face.hxx>
#include <NXOpen/Body.hxx>
#include <NXOpen/BasePart.hxx>
#include <NXOpen/Part.hxx>
#include <NXOpen/PartCollection.hxx>
#include <NXOpen/BodyCollection.hxx>
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

    double NxRoiResolver::ComputeFaceArea(NXOpen::Face* face)
    {
        // TODO(office-PC verify): UF_MODL_ask_mass_props_3d (via
        // UFSession::Modl.AskMassProps3d) is the documented low-level call
        // for surface area - NXOpen doesn't expose Area at the high level
        // on Face. Confirm the wrapper's exact parameter list/order and
        // output-array sizes against NX2406's uf_modl.h; density/accuracy
        // are required by the UF call even though they're meaningless for
        // a pure-surface (not solid-mass) query on a single Face.
        try
        {
            NXOpen::UF::UFSession* ufSession = NXOpen::UF::UFSession::GetUFSession();

            NXOpen::Tag objects[1] = {face->Tag()};
            double mass = 0.0;
            double centerOfGravity[3] = {0.0, 0.0, 0.0};
            double inertiaMatrix[9] = {0.0};
            double periphInertiaMatrix[9] = {0.0};
            double radiusOfGyration[3] = {0.0};
            double statMoment[3] = {0.0};
            double surfaceArea = 0.0;
            double surfaceAreaAccuracy = 0.0;
            double massAccuracy = 0.0;

            ufSession->Modl.AskMassProps3d(
                objects, 1,
                1.0,    // density - unused for a pure-surface query
                0.99,   // accuracy_desired
                &mass, centerOfGravity, inertiaMatrix, periphInertiaMatrix,
                radiusOfGyration, statMoment, &surfaceArea, &surfaceAreaAccuracy, &massAccuracy);

            return surfaceArea;
        }
        catch (const NXOpen::NXException& ex)
        {
            if (m_logger)
            {
                m_logger->Warn(std::string("ROI face area calculation failed: ") + ex.Message());
            }
            return 0.0;
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
            selection.area = ComputeFaceArea(face);

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

    OperationResult<std::vector<ROISelection>> NxRoiResolver::PromptSelectFacesInBox(const std::string& note)
    {
        if (m_sessionAccessor == nullptr || !m_sessionAccessor->IsAvailable())
        {
            return OperationResult<std::vector<ROISelection>>::Fail("NX session is not available - connect via CadImportModule first");
        }

        try
        {
            NXOpen::UI* ui = NXOpen::UI::GetUI();
            NXOpen::Selection* selectionManager = ui->SelectionManager();

            // Same Face-only mask as PromptSelectFace - see its TODO for the
            // field-name caveat on MaskTriple.
            NXOpen::Selection::MaskTriple faceMask;
            faceMask.Type = UF_solid_type;
            faceMask.Subtype = 0;
            faceMask.SolidBodySubtype = UF_UI_SEL_FEATURE_ANY_FACE;
            std::vector<NXOpen::Selection::MaskTriple> maskTriples{faceMask};

            std::vector<NXOpen::TaggedObject*> selectedObjects;
            NXOpen::Point3d cursorPosition;

            // TODO(office-PC verify): SelectTaggedObjects (plural) is the
            // rubber-band/box multi-pick counterpart to SelectTaggedObject
            // used in PromptSelectFace - a single drag in the graphics
            // window returns every object inside the box instead of one.
            // Confirm the exact overload (parameter count/order, and
            // whether the output parameter is really a
            // std::vector<TaggedObject*>*) against NX2406's Selection.hxx.
            NXOpen::Selection::Response response = selectionManager->SelectTaggedObjects(
                "Select ROI Faces (box)",
                "Drag a box to select multiple Faces for ROI",
                NXOpen::Selection::SelectionScopeWorkPart,
                NXOpen::Selection::SelectionActionClearAndEnableSpecific,
                false,
                false,
                maskTriples,
                &selectedObjects,
                &cursorPosition);

            const bool userCancelled =
                (response == NXOpen::Selection::ResponseCancel) ||
                (response == NXOpen::Selection::ResponseBack) ||
                selectedObjects.empty();

            if (userCancelled)
            {
                if (m_logger) m_logger->Info("ROI box selection cancelled by user");
                return OperationResult<std::vector<ROISelection>>::Fail("User cancelled ROI box selection");
            }

            std::vector<ROISelection> results;
            results.reserve(selectedObjects.size());

            for (NXOpen::TaggedObject* obj : selectedObjects)
            {
                NXOpen::Face* face = dynamic_cast<NXOpen::Face*>(obj);
                if (face == nullptr)
                {
                    continue;   // mask should guarantee Face-only; skip defensively
                }

                OperationResult<ROISelection> built = BuildSelectionFromFace(face, note);
                if (built.success)
                {
                    results.push_back(built.value);
                }
                else if (m_logger)
                {
                    m_logger->Warn("Skipped one face in box selection: " + built.errorMessage);
                }
            }

            if (results.empty())
            {
                return OperationResult<std::vector<ROISelection>>::Fail("Box selection returned no resolvable Faces");
            }

            if (m_logger)
            {
                m_logger->Info("ROI box selection resolved " + std::to_string(results.size()) + " face(s)");
            }

            return OperationResult<std::vector<ROISelection>>::Ok(std::move(results));
        }
        catch (const NXOpen::NXException& ex)
        {
            const std::string msg = std::string("NXException while prompting for ROI box selection: ") + ex.Message();
            if (m_logger) m_logger->Error(msg);
            return OperationResult<std::vector<ROISelection>>::Fail(msg);
        }
    }

    OperationResult<std::vector<ROISelection>> NxRoiResolver::PromptSelectBodiesInBox(const std::string& note)
    {
        if (m_sessionAccessor == nullptr || !m_sessionAccessor->IsAvailable())
        {
            return OperationResult<std::vector<ROISelection>>::Fail("NX session is not available - connect via CadImportModule first");
        }

        try
        {
            NXOpen::UI* ui = NXOpen::UI::GetUI();
            NXOpen::Selection* selectionManager = ui->SelectionManager();

            // Whole-Body mask (as opposed to PromptSelectFacesInBox's
            // per-Face mask) - this is what makes a "local ROI scope" a set
            // of whole Bodies rather than individual Faces. TODO(office-PC
            // verify): UF_UI_SEL_FEATURE_SOLID_BODY is the assumed constant
            // for "any solid body" in NXOpen::Selection::MaskTriple,
            // mirroring UF_UI_SEL_FEATURE_ANY_FACE used for Faces above -
            // confirm against NX2406's uf_ui_types.h.
            NXOpen::Selection::MaskTriple bodyMask;
            bodyMask.Type = UF_solid_type;
            bodyMask.Subtype = 0;
            bodyMask.SolidBodySubtype = UF_UI_SEL_FEATURE_SOLID_BODY;
            std::vector<NXOpen::Selection::MaskTriple> maskTriples{bodyMask};

            std::vector<NXOpen::TaggedObject*> selectedObjects;
            NXOpen::Point3d cursorPosition;

            // Same SelectTaggedObjects call as PromptSelectFacesInBox - see
            // its TODO for the overload/signature caveat. The Front/Back
            // view requirement (see IRoiResolver.h) is on the *caller*: NX
            // does the screen-space hit-test regardless of view, but a
            // rotated/oblique view would make "which Bodies fall in this
            // box" mean something harder to reason about for a local ROI
            // scope, so App only documents Front/Back as supported, it
            // doesn't (and can't, from in here) enforce it.
            NXOpen::Selection::Response response = selectionManager->SelectTaggedObjects(
                "Select ROI Bodies (box)",
                "Drag a box over the local region to scope this ROI to",
                NXOpen::Selection::SelectionScopeWorkPart,
                NXOpen::Selection::SelectionActionClearAndEnableSpecific,
                false,
                false,
                maskTriples,
                &selectedObjects,
                &cursorPosition);

            const bool userCancelled =
                (response == NXOpen::Selection::ResponseCancel) ||
                (response == NXOpen::Selection::ResponseBack) ||
                selectedObjects.empty();

            if (userCancelled)
            {
                if (m_logger) m_logger->Info("ROI body-box selection cancelled by user");
                return OperationResult<std::vector<ROISelection>>::Fail("User cancelled ROI body-box selection");
            }

            std::vector<ROISelection> results;
            results.reserve(selectedObjects.size());

            for (NXOpen::TaggedObject* obj : selectedObjects)
            {
                NXOpen::Body* body = dynamic_cast<NXOpen::Body*>(obj);
                if (body == nullptr)
                {
                    continue;   // mask should guarantee Body-only; skip defensively
                }

                OperationResult<ROISelection> built = BuildSelectionFromBodyAsComponent(body, note);
                if (built.success)
                {
                    results.push_back(built.value);
                }
                else if (m_logger)
                {
                    m_logger->Warn("Skipped one body in box selection: " + built.errorMessage);
                }
            }

            if (results.empty())
            {
                return OperationResult<std::vector<ROISelection>>::Fail("Box selection returned no resolvable Bodies");
            }

            if (m_logger)
            {
                m_logger->Info("ROI body-box selection resolved " + std::to_string(results.size()) + " body(s)");
            }

            return OperationResult<std::vector<ROISelection>>::Ok(std::move(results));
        }
        catch (const NXOpen::NXException& ex)
        {
            const std::string msg = std::string("NXException while prompting for ROI body-box selection: ") + ex.Message();
            if (m_logger) m_logger->Error(msg);
            return OperationResult<std::vector<ROISelection>>::Fail(msg);
        }
    }

    OperationResult<ROISelection> NxRoiResolver::ResolvePointAtCoordinate(const Point3D& coordinate, const std::string& note)
    {
        if (m_sessionAccessor == nullptr || !m_sessionAccessor->IsAvailable())
        {
            return OperationResult<ROISelection>::Fail("NX session is not available - connect via CadImportModule first");
        }

        try
        {
            NXOpen::Session* session = m_sessionAccessor->RawSession();
            NXOpen::BasePart* workPart = (session != nullptr) ? session->Parts()->Work() : nullptr;
            if (workPart == nullptr)
            {
                return OperationResult<ROISelection>::Fail("No active work part - cannot resolve coordinate");
            }

            double point[3] = {coordinate.x, coordinate.y, coordinate.z};
            NXOpen::UF::UFSession* ufSession = NXOpen::UF::UFSession::GetUFSession();

            // Every Body across the whole assembly, not just ones owned
            // directly by workPart (which is typically an empty container -
            // see NxContracts::CollectAllBodiesInWorkPart for why).
            const std::vector<NXOpen::Body*> allBodies = NxContracts::CollectAllBodiesInWorkPart(workPart);

            NXOpen::Body* matchedBody = nullptr;

            // TODO(office-PC verify): UF_MODL_ask_point_containment (via
            // UFSession::Modl.AskPointContainment) is the documented
            // point-in-solid test - confirm the wrapper's exact signature
            // and the UF_MODL_POINT_* enum values against NX2406.
            for (NXOpen::Body* candidate : allBodies)
            {
                int containment = 0;
                ufSession->Modl.AskPointContainment(point, candidate->Tag(), &containment);
                if (containment != 0 /* != UF_MODL_POINT_OUTSIDE */)
                {
                    matchedBody = candidate;
                    break;
                }
            }

            if (matchedBody == nullptr)
            {
                // Fallback: nearest body by bounding-box-center distance -
                // keeps a near-miss coordinate (e.g. authored slightly off
                // the BLU surface due to rounding) usable instead of just
                // failing outright.
                double bestDistanceSq = -1.0;
                for (NXOpen::Body* candidate : allBodies)
                {
                    const BoundingBox3D box = NxContracts::ComputeBoundingBoxForTag(candidate);
                    if (!box.valid)
                    {
                        continue;
                    }

                    const double cx = (box.min.x + box.max.x) / 2.0;
                    const double cy = (box.min.y + box.max.y) / 2.0;
                    const double cz = (box.min.z + box.max.z) / 2.0;
                    const double dx = cx - coordinate.x;
                    const double dy = cy - coordinate.y;
                    const double dz = cz - coordinate.z;
                    const double distanceSq = dx * dx + dy * dy + dz * dz;

                    if (bestDistanceSq < 0.0 || distanceSq < bestDistanceSq)
                    {
                        bestDistanceSq = distanceSq;
                        matchedBody = candidate;
                    }
                }
            }

            if (matchedBody == nullptr)
            {
                return OperationResult<ROISelection>::Fail("No bodies in work part to resolve coordinate against");
            }

            return BuildSelectionFromBody(matchedBody, coordinate, note);
        }
        catch (const NXOpen::NXException& ex)
        {
            const std::string msg = std::string("NXException while resolving ROI coordinate: ") + ex.Message();
            if (m_logger) m_logger->Error(msg);
            return OperationResult<ROISelection>::Fail(msg);
        }
    }

    OperationResult<ROISelection> NxRoiResolver::BuildSelectionFromBody(NXOpen::Body* body, const Point3D& coordinate, const std::string& note)
    {
        ROISelection selection;
        selection.kind = ROISelectionKind::Point;
        selection.note = note;
        selection.resolved = true;
        selection.coordinate = coordinate;

        try
        {
            selection.targetId = body->JournalIdentifier();
            selection.bodyId = body->JournalIdentifier();

            NXOpen::Assemblies::Component* owner = body->OwningComponent();
            selection.componentName = (owner != nullptr) ? owner->Name() : std::string();

            selection.boundingBox = NxContracts::ComputeBoundingBoxForTag(body);

            if (m_logger)
            {
                m_logger->Info("ROI coordinate resolved: component=" + selection.componentName +
                                " body=" + selection.bodyId);
            }

            return OperationResult<ROISelection>::Ok(selection);
        }
        catch (const NXOpen::NXException& ex)
        {
            const std::string msg = std::string("NXException while resolving ROI coordinate geometry: ") + ex.Message();
            if (m_logger) m_logger->Error(msg);
            return OperationResult<ROISelection>::Fail(msg);
        }
    }

    OperationResult<ROISelection> NxRoiResolver::BuildSelectionFromBodyAsComponent(NXOpen::Body* body, const std::string& note)
    {
        ROISelection selection;
        selection.kind = ROISelectionKind::Component;
        selection.note = note;
        selection.resolved = true;

        try
        {
            selection.targetId = body->JournalIdentifier();
            selection.bodyId = body->JournalIdentifier();

            NXOpen::Assemblies::Component* owner = body->OwningComponent();
            selection.componentName = (owner != nullptr) ? owner->Name() : std::string();

            selection.boundingBox = NxContracts::ComputeBoundingBoxForTag(body);

            if (m_logger)
            {
                m_logger->Info("ROI body resolved: component=" + selection.componentName +
                                " body=" + selection.bodyId);
            }

            return OperationResult<ROISelection>::Ok(selection);
        }
        catch (const NXOpen::NXException& ex)
        {
            const std::string msg = std::string("NXException while resolving ROI body geometry: ") + ex.Message();
            if (m_logger) m_logger->Error(msg);
            return OperationResult<ROISelection>::Fail(msg);
        }
    }

    OperationResult<FacetMesh> NxRoiResolver::ExtractFacetMesh(const ROISelection& selection)
    {
        if (!selection.resolved || selection.bodyId.empty())
        {
            return OperationResult<FacetMesh>::Fail("Selection is not resolved to a body - nothing to facet");
        }

        if (m_sessionAccessor == nullptr || !m_sessionAccessor->IsAvailable())
        {
            return OperationResult<FacetMesh>::Fail("NX session is not available - connect via CadImportModule first");
        }

        try
        {
            NXOpen::Session* session = m_sessionAccessor->RawSession();
            NXOpen::BasePart* workPart = (session != nullptr) ? session->Parts()->Work() : nullptr;
            if (workPart == nullptr)
            {
                return OperationResult<FacetMesh>::Fail("No active work part - cannot extract facets");
            }

            // ROISelection only stores JournalIdentifier strings (so Core
            // stays NXOpen-free) - re-resolve the live NXOpen::Body* by
            // linear search rather than assuming a direct "journal
            // identifier -> tag" lookup API we're not confident exists.
            // Searches the whole assembly (see NxContracts::
            // CollectAllBodiesInWorkPart), not just workPart's own Bodies().
            NXOpen::Body* targetBody = nullptr;
            for (NXOpen::Body* candidate : NxContracts::CollectAllBodiesInWorkPart(workPart))
            {
                if (candidate->JournalIdentifier() == selection.bodyId)
                {
                    targetBody = candidate;
                    break;
                }
            }

            if (targetBody == nullptr)
            {
                return OperationResult<FacetMesh>::Fail("Could not re-resolve bodyId='" + selection.bodyId + "' in the current work part");
            }

            if (selection.kind == ROISelectionKind::Face)
            {
                // Facet only the specific Face the user picked, not the
                // whole body - matches ROI scope (leakage analysis on one
                // surface, not the whole part).
                for (NXOpen::Face* candidateFace : targetBody->GetFaces())
                {
                    if (candidateFace->JournalIdentifier() == selection.targetId)
                    {
                        return FacetFace(candidateFace);
                    }
                }
                return OperationResult<FacetMesh>::Fail("Could not re-resolve Face targetId='" + selection.targetId + "' on body");
            }

            // Point-kind (resolved via coordinate) has no single Face to
            // point at - facet every face of the containing body and merge.
            FacetMesh combined;
            for (NXOpen::Face* candidateFace : targetBody->GetFaces())
            {
                OperationResult<FacetMesh> faceMesh = FacetFace(candidateFace);
                if (!faceMesh.success)
                {
                    continue;   // skip faces NX can't facet, keep the rest
                }

                const int indexOffset = static_cast<int>(combined.vertices.size());
                combined.vertices.insert(combined.vertices.end(), faceMesh.value.vertices.begin(), faceMesh.value.vertices.end());
                for (MeshTriangle triangle : faceMesh.value.triangles)
                {
                    triangle.v0 += indexOffset;
                    triangle.v1 += indexOffset;
                    triangle.v2 += indexOffset;
                    combined.triangles.push_back(triangle);
                }
            }
            combined.valid = !combined.triangles.empty();

            if (!combined.valid)
            {
                return OperationResult<FacetMesh>::Fail("No facetable faces found on body");
            }

            if (m_logger)
            {
                m_logger->Info("ROI facet mesh extracted: " + std::to_string(combined.triangles.size()) + " triangle(s)");
            }

            return OperationResult<FacetMesh>::Ok(combined);
        }
        catch (const NXOpen::NXException& ex)
        {
            const std::string msg = std::string("NXException while extracting ROI facet mesh: ") + ex.Message();
            if (m_logger) m_logger->Error(msg);
            return OperationResult<FacetMesh>::Fail(msg);
        }
    }

    OperationResult<FacetMesh> NxRoiResolver::FacetFace(NXOpen::Face* face)
    {
        // TODO(office-PC verify): UF_MODL_ask_face_facets (via
        // UFSession::Modl.AskFaceFacets) is the documented low-level
        // triangulation call used for STL-style export - confirm the exact
        // parameter list (chord-height/angle-tolerance units, whether
        // output arrays are really UF-allocated flat double*/int* buffers
        // in [x,y,z] / [v0,v1,v2] layout) against NX2406's uf_modl.h.
        // Highest-uncertainty call in this module: unlike AskBoundingBox/
        // AskMassProps3d/AskPointContainment (fixed-size in/out params),
        // this one hands back UF-owned heap arrays that must be freed with
        // the matching UF free function (assumed UF_free - verify the name
        // too) once copied into FacetMesh, or this leaks per call.
        double* points = nullptr;
        double* normals = nullptr;
        int* facets = nullptr;

        try
        {
            NXOpen::UF::UFSession* ufSession = NXOpen::UF::UFSession::GetUFSession();

            const double chordHeight = 0.01;      // mm - typical STL-export-quality default
            const double angleTolerance = 15.0;   // degrees

            int numPoints = 0;
            int numNormals = 0;
            int numFacets = 0;

            ufSession->Modl.AskFaceFacets(
                face->Tag(), chordHeight, angleTolerance,
                &numPoints, &points, &numNormals, &normals, &numFacets, &facets);

            FacetMesh mesh;
            mesh.vertices.reserve(static_cast<size_t>(numPoints));
            for (int i = 0; i < numPoints; ++i)
            {
                MeshVertex vertex;
                vertex.position = {points[i * 3 + 0], points[i * 3 + 1], points[i * 3 + 2]};
                vertex.normal = (i < numNormals)
                    ? Point3D{normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2]}
                    : Point3D{};
                mesh.vertices.push_back(vertex);
            }

            mesh.triangles.reserve(static_cast<size_t>(numFacets));
            for (int i = 0; i < numFacets; ++i)
            {
                MeshTriangle triangle;
                triangle.v0 = facets[i * 3 + 0];
                triangle.v1 = facets[i * 3 + 1];
                triangle.v2 = facets[i * 3 + 2];
                mesh.triangles.push_back(triangle);
            }
            mesh.valid = !mesh.triangles.empty();

            UF_free(points);
            UF_free(normals);
            UF_free(facets);

            if (!mesh.valid)
            {
                return OperationResult<FacetMesh>::Fail("Facet extraction returned zero triangles for this Face");
            }

            return OperationResult<FacetMesh>::Ok(mesh);
        }
        catch (const NXOpen::NXException& ex)
        {
            // Best-effort cleanup - a mid-call NXException may have left
            // one or more of these still null, UF_free(nullptr) must be a
            // no-op for this to be safe (verify alongside the rest of this
            // function).
            UF_free(points);
            UF_free(normals);
            UF_free(facets);

            const std::string msg = std::string("NXException while faceting Face: ") + ex.Message();
            if (m_logger) m_logger->Warn(msg);
            return OperationResult<FacetMesh>::Fail(msg);
        }
    }
}

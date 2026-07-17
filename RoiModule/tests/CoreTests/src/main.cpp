// RoiModule::Core unit tests. NXOpen-free by construction (Core has zero
// dependency on it) - this exercises ROIManager's bookkeeping and (via
// MockRoiResolver) RoiPickingWorkflows' orchestration logic, not any real
// geometry resolution (that's NxRoiResolver's job, and it needs a live NX
// session to test meaningfully - see RoiModule/README.md).

#include "MiniTest.h"
#include "MockRoiResolver.h"
#include "SpyLogger.h"

#include "Core/ROI/ROIManager.h"
#include "Core/ROI/RoiPickingWorkflows.h"

using namespace CadImport::Core;
using namespace CadImport::Core::Tests;

TEST_CASE(NewManager_HasNoSelections)
{
    ROIManager manager;
    CHECK(manager.GetSelections().empty());
}

TEST_CASE(SelectPoint_RecordsUnresolvedPointSelection)
{
    ROIManager manager;
    manager.SelectPoint("point-1", "corner note");

    const auto& selections = manager.GetSelections();
    CHECK(selections.size() == 1);
    CHECK(selections[0].kind == ROISelectionKind::Point);
    CHECK(selections[0].targetId == "point-1");
    CHECK(selections[0].note == "corner note");
    CHECK(selections[0].resolved == false);
}

TEST_CASE(SelectFace_RecordsUnresolvedFaceSelection)
{
    ROIManager manager;
    manager.SelectFace("face-1");

    const auto& selections = manager.GetSelections();
    CHECK(selections.size() == 1);
    CHECK(selections[0].kind == ROISelectionKind::Face);
    CHECK(selections[0].targetId == "face-1");
    CHECK(selections[0].resolved == false);
}

TEST_CASE(SelectComponent_RecordsUnresolvedComponentSelection)
{
    ROIManager manager;
    manager.SelectComponent("comp-1");

    const auto& selections = manager.GetSelections();
    CHECK(selections.size() == 1);
    CHECK(selections[0].kind == ROISelectionKind::Component);
    CHECK(selections[0].targetId == "comp-1");
    CHECK(selections[0].resolved == false);
}

TEST_CASE(SelectWithoutNote_DefaultsToEmptyNote)
{
    ROIManager manager;
    manager.SelectFace("face-no-note");

    CHECK(manager.GetSelections()[0].note.empty());
}

TEST_CASE(MixedSelections_PreserveInsertionOrder)
{
    ROIManager manager;
    manager.SelectPoint("p1");
    manager.SelectFace("f1");
    manager.SelectComponent("c1");
    manager.SelectFace("f2");

    const auto& selections = manager.GetSelections();
    CHECK(selections.size() == 4);
    CHECK(selections[0].targetId == "p1");
    CHECK(selections[1].targetId == "f1");
    CHECK(selections[2].targetId == "c1");
    CHECK(selections[3].targetId == "f2");
}

TEST_CASE(AddResolvedSelection_KeepsGeometryFieldsAsIs)
{
    ROIManager manager;

    ROISelection resolved;
    resolved.kind = ROISelectionKind::Face;
    resolved.targetId = "face-42";
    resolved.note = "roi note";
    resolved.resolved = true;
    resolved.componentName = "BLU_TOP";
    resolved.bodyId = "body-7";
    resolved.area = 123.45;
    resolved.boundingBox.valid = true;
    resolved.boundingBox.min = { 0.0, 0.0, 0.0 };
    resolved.boundingBox.max = { 10.0, 20.0, 30.0 };

    manager.AddResolvedSelection(resolved);

    const auto& selections = manager.GetSelections();
    CHECK(selections.size() == 1);
    CHECK(selections[0].resolved == true);
    CHECK(selections[0].componentName == "BLU_TOP");
    CHECK(selections[0].bodyId == "body-7");
    CHECK(selections[0].area == 123.45);
    CHECK(selections[0].boundingBox.valid == true);
    CHECK(selections[0].boundingBox.max.z == 30.0);
}

TEST_CASE(AddResolvedSelection_KeepsCoordinateForPointKind)
{
    ROIManager manager;

    ROISelection resolved;
    resolved.kind = ROISelectionKind::Point;
    resolved.targetId = "body-3";
    resolved.resolved = true;
    resolved.coordinate = { 1.5, -2.5, 3.0 };

    manager.AddResolvedSelection(resolved);

    const auto& selections = manager.GetSelections();
    CHECK(selections[0].coordinate.x == 1.5);
    CHECK(selections[0].coordinate.y == -2.5);
    CHECK(selections[0].coordinate.z == 3.0);
}

TEST_CASE(AddResolvedSelection_DoesNotRecomputeOrTouchOtherSelections)
{
    ROIManager manager;
    manager.SelectPoint("p1");

    ROISelection resolved;
    resolved.kind = ROISelectionKind::Face;
    resolved.targetId = "face-1";
    resolved.resolved = true;
    manager.AddResolvedSelection(resolved);

    const auto& selections = manager.GetSelections();
    CHECK(selections.size() == 2);
    CHECK(selections[0].resolved == false);   // untouched
    CHECK(selections[1].resolved == true);
}

TEST_CASE(ClearSelections_EmptiesList)
{
    ROIManager manager;
    manager.SelectPoint("p1");
    manager.SelectFace("f1");
    manager.ClearSelections();

    CHECK(manager.GetSelections().empty());
}

TEST_CASE(SelectFace_LogsDebugMessageWithTargetId)
{
    SpyLogger logger;
    ROIManager manager(&logger);
    manager.SelectFace("f-99");

    CHECK(logger.debugMessages.size() == 1);
    CHECK(logger.debugMessages[0].find("f-99") != std::string::npos);
    CHECK(logger.infoMessages.empty());
}

TEST_CASE(AddResolvedSelection_LogsInfoMessageWithComponentAndArea)
{
    SpyLogger logger;
    ROIManager manager(&logger);

    ROISelection resolved;
    resolved.targetId = "face-1";
    resolved.componentName = "BLU_TOP";
    resolved.area = 55.5;
    manager.AddResolvedSelection(resolved);

    CHECK(logger.infoMessages.size() == 1);
    CHECK(logger.infoMessages[0].find("BLU_TOP") != std::string::npos);
    CHECK(logger.infoMessages[0].find("face-1") != std::string::npos);
}

TEST_CASE(ClearSelections_LogsDebugMessage)
{
    SpyLogger logger;
    ROIManager manager(&logger);
    manager.SelectPoint("p1");
    logger.debugMessages.clear();   // isolate the Clear call from SelectPoint's own log

    manager.ClearSelections();

    CHECK(logger.debugMessages.size() == 1);
}

TEST_CASE(NullLogger_DoesNotCrash)
{
    ROIManager manager(nullptr);
    manager.SelectPoint("p1");
    manager.AddResolvedSelection(ROISelection{});
    manager.ClearSelections();

    CHECK(true);   // reaching this line without crashing is the assertion
}

// ---- RoiPickingWorkflows (via MockRoiResolver) ----------------------------
// No live NX session anywhere below - MockRoiResolver stands in for
// NxRoiResolver, so these exercise exactly the orchestration logic that
// App/src/main.cpp's picking loops call, including the "which flow tags
// scopeId" and "does attachMesh actually call ExtractFacetMesh" behavior
// that isn't NX-specific in the first place.

namespace
{
    ROISelection MakeResolvedComponent(const std::string& bodyId)
    {
        ROISelection s;
        s.kind = ROISelectionKind::Component;
        s.resolved = true;
        s.targetId = bodyId;
        s.bodyId = bodyId;
        s.componentName = bodyId + "_COMP";
        return s;
    }
}

TEST_CASE(TryAddBodySelectionsInBox_TagsAllResultsWithScopeId)
{
    MockRoiResolver resolver;
    resolver.bodyBoxResult = OperationResult<std::vector<ROISelection>>::Ok(
        {MakeResolvedComponent("body-1"), MakeResolvedComponent("body-2")});

    ROIManager manager;
    int count = TryAddBodySelectionsInBox(resolver, manager, "bottom-corner", "", false, nullptr);

    CHECK(count == 2);
    CHECK(manager.GetSelections().size() == 2);
    CHECK(manager.GetSelections()[0].scopeId == "bottom-corner");
    CHECK(manager.GetSelections()[1].scopeId == "bottom-corner");
}

TEST_CASE(TryAddBodySelectionsInBox_TwoScopesStayDistinct)
{
    MockRoiResolver resolver;
    ROIManager manager;

    resolver.bodyBoxResult = OperationResult<std::vector<ROISelection>>::Ok({MakeResolvedComponent("body-1")});
    TryAddBodySelectionsInBox(resolver, manager, "bottom-corner", "", false, nullptr);

    resolver.bodyBoxResult = OperationResult<std::vector<ROISelection>>::Ok({MakeResolvedComponent("body-2"), MakeResolvedComponent("body-3")});
    TryAddBodySelectionsInBox(resolver, manager, "back-center", "", false, nullptr);

    const auto& selections = manager.GetSelections();
    CHECK(selections.size() == 3);
    CHECK(selections[0].scopeId == "bottom-corner");
    CHECK(selections[1].scopeId == "back-center");
    CHECK(selections[2].scopeId == "back-center");
}

TEST_CASE(TryAddBodySelectionsInBox_ReturnsZero_AndAddsNothing_WhenResolverFails)
{
    MockRoiResolver resolver;
    resolver.bodyBoxResult = OperationResult<std::vector<ROISelection>>::Fail("cancelled");

    ROIManager manager;
    int count = TryAddBodySelectionsInBox(resolver, manager, "bottom-corner", "", false, nullptr);

    CHECK(count == 0);
    CHECK(manager.GetSelections().empty());
}

TEST_CASE(TryAddBodySelectionsInBox_AttachesMesh_WhenRequested)
{
    MockRoiResolver resolver;
    resolver.bodyBoxResult = OperationResult<std::vector<ROISelection>>::Ok({MakeResolvedComponent("body-1")});
    FacetMesh mesh;
    mesh.valid = true;
    mesh.triangles.push_back(MeshTriangle{});
    resolver.meshResult = OperationResult<FacetMesh>::Ok(mesh);

    ROIManager manager;
    TryAddBodySelectionsInBox(resolver, manager, "bottom-corner", "", true, nullptr);

    CHECK(resolver.meshCalls == 1);
    CHECK(manager.GetSelections()[0].mesh.valid == true);
    CHECK(manager.GetSelections()[0].mesh.triangles.size() == 1);
}

TEST_CASE(TryAddBodySelectionsInBox_DoesNotCallExtractFacetMesh_WhenNotRequested)
{
    MockRoiResolver resolver;
    resolver.bodyBoxResult = OperationResult<std::vector<ROISelection>>::Ok({MakeResolvedComponent("body-1")});

    ROIManager manager;
    TryAddBodySelectionsInBox(resolver, manager, "bottom-corner", "", false, nullptr);

    CHECK(resolver.meshCalls == 0);
    CHECK(manager.GetSelections()[0].mesh.valid == false);
}

TEST_CASE(TryAddFaceSelection_AddsSelection_OnSuccess)
{
    MockRoiResolver resolver;
    resolver.faceResult = OperationResult<ROISelection>::Ok(MakeResolvedComponent("body-1"));

    ROIManager manager;
    bool ok = TryAddFaceSelection(resolver, manager, "some note", false, nullptr);

    CHECK(ok == true);
    CHECK(resolver.lastNote == "some note");
    CHECK(manager.GetSelections().size() == 1);
}

TEST_CASE(TryAddFaceSelection_ReturnsFalse_AndAddsNothing_OnFailure)
{
    MockRoiResolver resolver;
    resolver.faceResult = OperationResult<ROISelection>::Fail("user cancelled");

    ROIManager manager;
    bool ok = TryAddFaceSelection(resolver, manager, "", false, nullptr);

    CHECK(ok == false);
    CHECK(manager.GetSelections().empty());
}

TEST_CASE(TryAddBoxSelections_AddsAllResults)
{
    MockRoiResolver resolver;
    resolver.boxResult = OperationResult<std::vector<ROISelection>>::Ok(
        {MakeResolvedComponent("f1"), MakeResolvedComponent("f2"), MakeResolvedComponent("f3")});

    ROIManager manager;
    int count = TryAddBoxSelections(resolver, manager, "", false, nullptr);

    CHECK(count == 3);
    CHECK(manager.GetSelections().size() == 3);
    // unlike TryAddBodySelectionsInBox, this path never sets scopeId
    CHECK(manager.GetSelections()[0].scopeId.empty());
}

TEST_CASE(TryAddPointSelection_PassesCoordinate_AndAddsSelection)
{
    MockRoiResolver resolver;
    resolver.pointResult = OperationResult<ROISelection>::Ok(MakeResolvedComponent("body-1"));

    ROIManager manager;
    bool ok = TryAddPointSelection(resolver, manager, Point3D{1.0, 2.0, 3.0}, "corner", false, nullptr);

    CHECK(ok == true);
    CHECK(resolver.pointCalls == 1);
    CHECK(resolver.lastNote == "corner");
    CHECK(manager.GetSelections().size() == 1);
}

TEST_CASE(TryAttachFacetMesh_SetsMesh_OnSuccess)
{
    MockRoiResolver resolver;
    FacetMesh mesh;
    mesh.valid = true;
    resolver.meshResult = OperationResult<FacetMesh>::Ok(mesh);

    ROISelection selection = MakeResolvedComponent("body-1");
    bool ok = TryAttachFacetMesh(resolver, selection, nullptr);

    CHECK(ok == true);
    CHECK(selection.mesh.valid == true);
}

TEST_CASE(TryAttachFacetMesh_LeavesSelectionUntouched_OnFailure)
{
    MockRoiResolver resolver;
    resolver.meshResult = OperationResult<FacetMesh>::Fail("degenerate face");

    ROISelection selection = MakeResolvedComponent("body-1");
    bool ok = TryAttachFacetMesh(resolver, selection, nullptr);

    CHECK(ok == false);
    CHECK(selection.mesh.valid == false);
    CHECK(selection.bodyId == "body-1");   // rest of the selection is untouched
}

int main()
{
    return MiniTest::RunAll();
}

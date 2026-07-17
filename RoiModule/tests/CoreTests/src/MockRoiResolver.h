#pragma once

#include <string>
#include <vector>

#include "Core/Interfaces/IRoiResolver.h"

namespace CadImport::Core::Tests
{
    // Fake IRoiResolver for exercising RoiPickingWorkflows (and, by
    // extension, App/src/main.cpp's picking loops) without a live NX
    // session. Each Prompt*/Resolve*/Extract* call returns whatever the
    // test pre-loaded into the matching *Result field, and bumps the
    // matching *Calls counter so tests can assert on call count/order too.
    class MockRoiResolver : public IRoiResolver
    {
    public:
        OperationResult<ROISelection> faceResult = OperationResult<ROISelection>::Fail("not configured");
        OperationResult<std::vector<ROISelection>> boxResult = OperationResult<std::vector<ROISelection>>::Fail("not configured");
        OperationResult<std::vector<ROISelection>> bodyBoxResult = OperationResult<std::vector<ROISelection>>::Fail("not configured");
        OperationResult<ROISelection> pointResult = OperationResult<ROISelection>::Fail("not configured");
        OperationResult<FacetMesh> meshResult = OperationResult<FacetMesh>::Fail("not configured");

        int faceCalls = 0;
        int boxCalls = 0;
        int bodyBoxCalls = 0;
        int pointCalls = 0;
        int meshCalls = 0;
        std::string lastNote;

        OperationResult<ROISelection> PromptSelectFace(const std::string& note) override
        {
            ++faceCalls;
            lastNote = note;
            return faceResult;
        }

        OperationResult<std::vector<ROISelection>> PromptSelectFacesInBox(const std::string& note) override
        {
            ++boxCalls;
            lastNote = note;
            return boxResult;
        }

        OperationResult<std::vector<ROISelection>> PromptSelectBodiesInBox(const std::string& note) override
        {
            ++bodyBoxCalls;
            lastNote = note;
            return bodyBoxResult;
        }

        OperationResult<ROISelection> ResolvePointAtCoordinate(const Point3D&, const std::string& note) override
        {
            ++pointCalls;
            lastNote = note;
            return pointResult;
        }

        OperationResult<FacetMesh> ExtractFacetMesh(const ROISelection&) override
        {
            ++meshCalls;
            return meshResult;
        }
    };
}

#include "NxBackend/NxGeometryReader.h"

#include <vector>

#include <NXOpen/Session.hxx>
#include <NXOpen/BasePart.hxx>
#include <NXOpen/Body.hxx>
#include <NXOpen/Face.hxx>
#include <NXOpen/Edge.hxx>
#include <NXOpen/NXException.hxx>
#include <NXOpen/UF/UFSession.hxx>
#include <NXOpen/Assemblies/Component.hxx>

using namespace CadImport::Core;

namespace CadImport::NxBackend
{
    NxGeometryReader::NxGeometryReader(NxConnector* connector, Core::ILogger* logger)
        : m_connector(connector), m_logger(logger)
    {
    }

    OperationResult<GeometryInfo> NxGeometryReader::ReadGeometry(const std::string& componentName)
    {
        if (m_connector == nullptr || !m_connector->IsAvailable())
        {
            return OperationResult<GeometryInfo>::Fail("NxConnector is not connected - call Connect() first");
        }

        try
        {
            NXOpen::Session* session = m_connector->RawSession();
            NXOpen::BasePart* workPart = session->Parts()->Work();
            if (workPart == nullptr)
            {
                return OperationResult<GeometryInfo>::Fail("No Work Part is currently loaded");
            }

            GeometryInfo geometryInfo;
            geometryInfo.componentName = componentName;

            // Display-only summary pass over the Work Part's bodies. Bodies
            // are filtered by owning component name.
            //
            // TODO(office-PC verify): Body::OwningComponent() is the
            // expected NX Open accessor for "which assembly occurrence does
            // this body belong to" - confirm the exact name against the
            // installed Body.hxx. For a non-assembly (single-part) bookmark
            // this filter is skipped and all bodies are reported instead.
            std::vector<NXOpen::Body*> bodies = workPart->Bodies();
            for (NXOpen::Body* body : bodies)
            {
                bool includeBody = true;
                NXOpen::Assemblies::Component* owner = body->OwningComponent();
                if (owner != nullptr)
                {
                    includeBody = (owner->Name() == componentName);
                }

                if (!includeBody)
                {
                    continue;
                }

                BodyInfo bodyInfo;
                bodyInfo.bodyId = body->JournalIdentifier();
                bodyInfo.faceCount = static_cast<int>(body->GetFaces().size());
                bodyInfo.edgeCount = static_cast<int>(body->GetEdges().size());
                bodyInfo.boundingBox = ComputeBoundingBox(body);

                geometryInfo.bodies.push_back(bodyInfo);
            }

            if (m_logger)
            {
                m_logger->Info("GeometryReader: component=" + componentName +
                                " bodies=" + std::to_string(geometryInfo.bodies.size()));
            }

            return OperationResult<GeometryInfo>::Ok(geometryInfo);
        }
        catch (const NXOpen::NXException& ex)
        {
            const std::string msg = std::string("NXException while reading geometry: ") + ex.Message();
            if (m_logger) m_logger->Error(msg);
            return OperationResult<GeometryInfo>::Fail(msg);
        }
    }

    BoundingBox3D NxGeometryReader::ComputeBoundingBox(NXOpen::Body* body)
    {
        BoundingBox3D box;

        // TODO(office-PC verify): Bounding box is most reliably available
        // via the low-level UF (User Function) bridge rather than a direct
        // high-level NXOpen::Body method. Confirm the exact UF_MODL
        // function name/signature (candidates: AskBoundingBox,
        // AskBoundingBoxExact) and the tag-retrieval call
        // (body->Tag() vs a dedicated conversion) against NX2406.
        try
        {
            NXOpen::UF::UFSession* ufSession = NXOpen::UF::UFSession::GetUFSession();
            double range[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            ufSession->Modl.AskBoundingBox(body->Tag(), range);

            box.min = { range[0], range[1], range[2] };
            box.max = { range[3], range[4], range[5] };
            box.valid = true;
        }
        catch (...)
        {
            box.valid = false;
        }

        return box;
    }
}

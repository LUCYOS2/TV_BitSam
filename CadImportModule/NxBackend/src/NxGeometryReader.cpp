#include "NxBackend/NxGeometryReader.h"

#include <vector>

#include <NXOpen/Session.hxx>
#include <NXOpen/BasePart.hxx>
#include <NXOpen/Body.hxx>
#include <NXOpen/Face.hxx>
#include <NXOpen/Edge.hxx>
#include <NXOpen/NXException.hxx>
#include <NXOpen/Assemblies/Component.hxx>

#include "NxContracts/NxGeometryUtils.h"

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
        // Shared with RoiModule's NxRoiResolver - see
        // Shared/NxContracts/NxGeometryUtils.h/.cpp for the actual UF call
        // and its TODO(office-PC verify) notes.
        return NxContracts::ComputeBoundingBoxForTag(body);
    }
}

#include "NxBackend/NxMaterialReader.h"

#include <NXOpen/Session.hxx>
#include <NXOpen/BasePart.hxx>
#include <NXOpen/Part.hxx>
#include <NXOpen/Body.hxx>
#include <NXOpen/PhysicalMaterial.hxx>
#include <NXOpen/PhysicalMaterialCollection.hxx>
#include <NXOpen/NXException.hxx>
#include <NXOpen/Assemblies/Component.hxx>

using namespace CadImport::Core;

namespace CadImport::NxBackend
{
    NxMaterialReader::NxMaterialReader(NxConnector* connector, Core::ILogger* logger)
        : m_connector(connector), m_logger(logger)
    {
    }

    OperationResult<std::vector<MaterialInfo>> NxMaterialReader::ReadMaterial(const std::string& componentName)
    {
        if (m_connector == nullptr || !m_connector->IsAvailable())
        {
            return OperationResult<std::vector<MaterialInfo>>::Fail("NxConnector is not connected - call Connect() first");
        }

        try
        {
            NXOpen::Session* session = m_connector->RawSession();
            NXOpen::BasePart* workPart = session->Parts()->Work();
            if (workPart == nullptr)
            {
                return OperationResult<std::vector<MaterialInfo>>::Fail("No Work Part is currently loaded");
            }

            std::vector<MaterialInfo> results;

            // TODO(office-PC verify): PhysicalMaterials() is exposed on
            // Part (not BasePart) in most NX versions, and
            // PhysicalMaterial::GetBodies() is a best-effort guess at the
            // accessor that returns bodies a material is assigned to -
            // confirm both against NX2406's Part.hxx / PhysicalMaterial.hxx.
            // If GetBodies() does not exist, the documented fallback is the
            // low-level `UF_SF_ask_body_material` (via UFSession::Sf) using
            // each body's tag - mirror the UF pattern used in
            // NxGeometryReader::ComputeBoundingBox.
            NXOpen::PhysicalMaterialCollection* materials = workPart->PhysicalMaterials();
            for (NXOpen::PhysicalMaterial* material : *materials)
            {
                std::vector<NXOpen::Body*> assignedBodies = material->GetBodies();
                for (NXOpen::Body* body : assignedBodies)
                {
                    NXOpen::Assemblies::Component* owner = body->OwningComponent();
                    const bool matches = (owner == nullptr) || (owner->Name() == componentName);
                    if (!matches)
                    {
                        continue;
                    }

                    MaterialInfo info;
                    info.componentName = componentName;
                    info.bodyId = body->JournalIdentifier();
                    info.materialName = material->Name();
                    results.push_back(info);
                }
            }

            if (m_logger)
            {
                m_logger->Info("MaterialReader: component=" + componentName +
                                " materials=" + std::to_string(results.size()));
            }

            return OperationResult<std::vector<MaterialInfo>>::Ok(results);
        }
        catch (const NXOpen::NXException& ex)
        {
            const std::string msg = std::string("NXException while reading materials: ") + ex.Message();
            if (m_logger) m_logger->Error(msg);
            return OperationResult<std::vector<MaterialInfo>>::Fail(msg);
        }
    }
}

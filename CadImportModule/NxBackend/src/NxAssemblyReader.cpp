#include "NxBackend/NxAssemblyReader.h"

#include <vector>

#include <NXOpen/Session.hxx>
#include <NXOpen/BasePart.hxx>
#include <NXOpen/Part.hxx>
#include <NXOpen/Assemblies/ComponentAssembly.hxx>
#include <NXOpen/Assemblies/Component.hxx>
#include <NXOpen/NXException.hxx>

using namespace CadImport::Core;

namespace CadImport::NxBackend
{
    NxAssemblyReader::NxAssemblyReader(NxConnector* connector, Core::ILogger* logger)
        : m_connector(connector), m_logger(logger)
    {
    }

    OperationResult<ComponentInfo> NxAssemblyReader::ReadTree()
    {
        if (m_connector == nullptr || !m_connector->IsAvailable())
        {
            return OperationResult<ComponentInfo>::Fail("NxConnector is not connected - call Connect() first");
        }

        try
        {
            NXOpen::Session* session = m_connector->RawSession();
            NXOpen::BasePart* workPart = session->Parts()->Work();
            if (workPart == nullptr)
            {
                return OperationResult<ComponentInfo>::Fail("No Work Part is currently loaded");
            }

            // ComponentAssembly is exposed on Part (not BasePart) in most
            // NX versions - TODO(office-PC verify): if NX2406 requires a
            // downcast (dynamic_cast<NXOpen::Part*>(workPart)), add it here.
            NXOpen::Assemblies::ComponentAssembly* assembly = workPart->ComponentAssembly();
            if (assembly == nullptr)
            {
                if (m_logger) m_logger->Warn("Work Part has no ComponentAssembly (not an assembly, or single-part file)");
                ComponentInfo single;
                single.name = workPart->Name();
                single.partName = workPart->Name();
                return OperationResult<ComponentInfo>::Ok(single);
            }

            NXOpen::Assemblies::Component* root = assembly->RootComponent();
            if (root == nullptr)
            {
                return OperationResult<ComponentInfo>::Fail("ComponentAssembly has no RootComponent");
            }

            ComponentInfo rootInfo = BuildComponentInfo(root, "");
            return OperationResult<ComponentInfo>::Ok(rootInfo);
        }
        catch (const NXOpen::NXException& ex)
        {
            const std::string msg = std::string("NXException while reading assembly tree: ") + ex.Message();
            if (m_logger) m_logger->Error(msg);
            return OperationResult<ComponentInfo>::Fail(msg);
        }
    }

    ComponentInfo NxAssemblyReader::BuildComponentInfo(NXOpen::Assemblies::Component* component, const std::string& parentName)
    {
        ComponentInfo info;
        info.name = component->Name();
        info.parentName = parentName;

        // TODO(office-PC verify): exact accessor names below. Common
        // candidates seen across NX Open versions:
        //   - Part name:      component->Prototype()->OwningPart()->Name()
        //   - Visibility:     component->IsBlanked() / session->Layer visibility
        //   - Suppression:    component->IsSuppressed() / SuppressionStatus()
        // Left as best-effort calls; adjust against the installed NX2406
        // Component.hxx once available.
        info.partName = component->Name();
        info.visible = true;
        info.suppressed = false;

        std::vector<NXOpen::Assemblies::Component*> children = component->GetChildren();
        info.children.reserve(children.size());
        for (NXOpen::Assemblies::Component* child : children)
        {
            info.children.push_back(BuildComponentInfo(child, info.name));
        }

        return info;
    }
}

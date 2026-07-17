#include "Export/JsonSceneExporter.h"

#include <algorithm>
#include <fstream>
#include <utility>

#include "Export/JsonWriter.h"

using namespace CadImport::Core;

namespace CadImport::App
{
    namespace
    {
        void WriteBoundingBox(JsonWriter& writer, const BoundingBox3D& box)
        {
            writer.Key("boundingBox");
            writer.BeginObject();
            writer.Key("valid");
            writer.Value(box.valid);
            writer.Key("min");
            writer.BeginObject();
            writer.Key("x"); writer.Value(box.min.x);
            writer.Key("y"); writer.Value(box.min.y);
            writer.Key("z"); writer.Value(box.min.z);
            writer.EndObject();
            writer.Key("max");
            writer.BeginObject();
            writer.Key("x"); writer.Value(box.max.x);
            writer.Key("y"); writer.Value(box.max.y);
            writer.Key("z"); writer.Value(box.max.z);
            writer.EndObject();
            writer.EndObject();
        }

        const char* SelectionKindToString(ROISelectionKind kind)
        {
            switch (kind)
            {
            case ROISelectionKind::Point: return "Point";
            case ROISelectionKind::Face: return "Face";
            case ROISelectionKind::Component: return "Component";
            }
            return "Unknown";
        }

        void WritePoint3D(JsonWriter& writer, const Point3D& point)
        {
            writer.BeginObject();
            writer.Key("x"); writer.Value(point.x);
            writer.Key("y"); writer.Value(point.y);
            writer.Key("z"); writer.Value(point.z);
            writer.EndObject();
        }

        // Only emitted when mesh.valid - most selections never call
        // IRoiResolver::ExtractFacetMesh (it's opt-in/expensive, see
        // ROISelection::mesh), so this keeps scene.json small by default.
        void WriteFacetMesh(JsonWriter& writer, const FacetMesh& mesh)
        {
            writer.Key("mesh");
            writer.BeginObject();
            writer.Key("valid"); writer.Value(mesh.valid);

            writer.Key("vertices");
            writer.BeginArray();
            for (const auto& vertex : mesh.vertices)
            {
                writer.BeginObject();
                writer.Key("position"); WritePoint3D(writer, vertex.position);
                writer.Key("normal"); WritePoint3D(writer, vertex.normal);
                writer.EndObject();
            }
            writer.EndArray();

            writer.Key("triangles");
            writer.BeginArray();
            for (const auto& triangle : mesh.triangles)
            {
                writer.BeginArray();
                writer.Value(triangle.v0);
                writer.Value(triangle.v1);
                writer.Value(triangle.v2);
                writer.EndArray();
            }
            writer.EndArray();

            writer.EndObject();
        }

        void WriteSelection(JsonWriter& writer, const ROISelection& sel)
        {
            writer.BeginObject();
            writer.Key("kind"); writer.Value(SelectionKindToString(sel.kind));
            writer.Key("targetId"); writer.Value(sel.targetId);
            writer.Key("note"); writer.Value(sel.note);
            writer.Key("resolved"); writer.Value(sel.resolved);
            writer.Key("componentName"); writer.Value(sel.componentName);
            writer.Key("bodyId"); writer.Value(sel.bodyId);
            writer.Key("area"); writer.Value(sel.area);
            WriteBoundingBox(writer, sel.boundingBox);
            writer.Key("coordinate"); WritePoint3D(writer, sel.coordinate);
            WriteFacetMesh(writer, sel.mesh);
            writer.EndObject();
        }

        // Groups selections by ROISelection::scopeId, preserving the order
        // each scopeId was first seen in (not sorted) - matches the order
        // the user picked local ROI scopes in during --roi-scope. Selections
        // with an empty scopeId are the caller's problem, not this
        // function's: see ToJson, which only passes the non-empty ones here.
        std::vector<std::pair<std::string, std::vector<ROISelection>>> GroupByScope(const std::vector<ROISelection>& selections)
        {
            std::vector<std::pair<std::string, std::vector<ROISelection>>> scopes;
            for (const auto& sel : selections)
            {
                auto existing = std::find_if(scopes.begin(), scopes.end(),
                    [&sel](const auto& scope) { return scope.first == sel.scopeId; });
                if (existing != scopes.end())
                {
                    existing->second.push_back(sel);
                }
                else
                {
                    scopes.push_back({sel.scopeId, std::vector<ROISelection>{sel}});
                }
            }
            return scopes;
        }
    }

    void JsonSceneExporter::WriteComponent(JsonWriter& writer, const ComponentInfo& component)
    {
        writer.BeginObject();
        writer.Key("name"); writer.Value(component.name);
        writer.Key("partName"); writer.Value(component.partName);
        writer.Key("parentName"); writer.Value(component.parentName);
        writer.Key("visible"); writer.Value(component.visible);
        writer.Key("suppressed"); writer.Value(component.suppressed);
        writer.Key("children");
        writer.BeginArray();
        for (const auto& child : component.children)
        {
            WriteComponent(writer, child);
        }
        writer.EndArray();
        writer.EndObject();
    }

    std::string JsonSceneExporter::ToJson(
        const SessionInfo& sessionInfo,
        const ComponentInfo& rootComponent,
        const std::vector<GeometryInfo>& geometry,
        const std::vector<MaterialInfo>& materials,
        const std::vector<ROISelection>& roiSelections) const
    {
        JsonWriter writer;

        writer.BeginObject();

        writer.Key("sessionInfo");
        writer.BeginObject();
        writer.Key("modelName"); writer.Value(sessionInfo.modelName);
        writer.Key("revision"); writer.Value(sessionInfo.revision);
        writer.Key("partName"); writer.Value(sessionInfo.partName);
        writer.Key("unit"); writer.Value(sessionInfo.unit);
        writer.EndObject();

        writer.Key("components");
        writer.BeginArray();
        WriteComponent(writer, rootComponent);
        writer.EndArray();

        writer.Key("geometry");
        writer.BeginArray();
        for (const auto& geo : geometry)
        {
            writer.BeginObject();
            writer.Key("componentName"); writer.Value(geo.componentName);
            writer.Key("bodies");
            writer.BeginArray();
            for (const auto& body : geo.bodies)
            {
                writer.BeginObject();
                writer.Key("bodyId"); writer.Value(body.bodyId);
                writer.Key("faceCount"); writer.Value(body.faceCount);
                writer.Key("edgeCount"); writer.Value(body.edgeCount);
                WriteBoundingBox(writer, body.boundingBox);
                writer.EndObject();
            }
            writer.EndArray();
            writer.EndObject();
        }
        writer.EndArray();

        writer.Key("materials");
        writer.BeginArray();
        for (const auto& mat : materials)
        {
            writer.BeginObject();
            writer.Key("componentName"); writer.Value(mat.componentName);
            writer.Key("bodyId"); writer.Value(mat.bodyId);
            writer.Key("materialName"); writer.Value(mat.materialName);
            writer.EndObject();
        }
        writer.EndArray();

        // Unscoped picks (--pick-roi / --pick-roi-box / --roi-point) land
        // here, flat, as before. Scoped picks (--roi-scope) are grouped
        // into roiScopes below instead - each local ROI pass exported as
        // its own independent unit, matching how they were picked.
        std::vector<ROISelection> unscoped;
        std::vector<ROISelection> scoped;
        for (const auto& sel : roiSelections)
        {
            (sel.scopeId.empty() ? unscoped : scoped).push_back(sel);
        }

        writer.Key("roiSelections");
        writer.BeginArray();
        for (const auto& sel : unscoped)
        {
            WriteSelection(writer, sel);
        }
        writer.EndArray();

        writer.Key("roiScopes");
        writer.BeginArray();
        for (const auto& scope : GroupByScope(scoped))
        {
            writer.BeginObject();
            writer.Key("scopeId"); writer.Value(scope.first);
            writer.Key("selections");
            writer.BeginArray();
            for (const auto& sel : scope.second)
            {
                WriteSelection(writer, sel);
            }
            writer.EndArray();
            writer.EndObject();
        }
        writer.EndArray();

        writer.EndObject();

        return writer.ToString();
    }

    OperationResult<bool> JsonSceneExporter::WriteToFile(
        const std::string& filePath,
        const SessionInfo& sessionInfo,
        const ComponentInfo& rootComponent,
        const std::vector<GeometryInfo>& geometry,
        const std::vector<MaterialInfo>& materials,
        const std::vector<ROISelection>& roiSelections) const
    {
        const std::string json = ToJson(sessionInfo, rootComponent, geometry, materials, roiSelections);

        std::ofstream file(filePath, std::ios::out | std::ios::trunc);
        if (!file.is_open())
        {
            return OperationResult<bool>::Fail("Failed to open file for writing: " + filePath);
        }

        file << json;
        if (!file.good())
        {
            return OperationResult<bool>::Fail("Failed while writing JSON to: " + filePath);
        }

        return OperationResult<bool>::Ok(true);
    }
}

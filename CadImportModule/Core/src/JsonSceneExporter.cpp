#include "Core/Export/JsonSceneExporter.h"

#include <fstream>

#include "Core/Export/JsonWriter.h"

namespace CadImport::Core
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

        writer.Key("roiSelections");
        writer.BeginArray();
        for (const auto& sel : roiSelections)
        {
            writer.BeginObject();
            writer.Key("kind"); writer.Value(SelectionKindToString(sel.kind));
            writer.Key("targetId"); writer.Value(sel.targetId);
            writer.Key("note"); writer.Value(sel.note);
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

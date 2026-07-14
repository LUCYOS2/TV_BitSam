// CAD Import Module - CLI entry point.
//
// This is an External EXE (see docs/DevGuide.md for why): it acquires its
// OWN NX session via NxConnector, opens the bookmark (.plmxl) passed on the
// command line, reads the Assembly Tree / Geometry summary / Material
// names, and writes a JSON Scene Manifest that downstream modules
// (Material Engine, Ray Tracing) can consume.
//
// Usage:
//   CadImportModule.exe --bookmark <path.plmxl> [--output <scene.json>]

#include <string>
#include <vector>

#include "Core/Logging/ConsoleLogger.h"
#include "Core/ROI/ROIManager.h"
#include "Core/Export/JsonSceneExporter.h"
#include "Core/Models/ComponentInfo.h"

#include "NxBackend/NxConnector.h"
#include "NxBackend/NxAssemblyReader.h"
#include "NxBackend/NxGeometryReader.h"
#include "NxBackend/NxMaterialReader.h"

using namespace CadImport::Core;
using namespace CadImport::NxBackend;

namespace
{
    struct CliArgs
    {
        std::string bookmarkPath;
        std::string outputPath = "scene.json";
        bool valid = false;
    };

    CliArgs ParseArgs(int argc, char** argv)
    {
        CliArgs args;
        for (int i = 1; i < argc; ++i)
        {
            const std::string arg = argv[i];
            if (arg == "--bookmark" && i + 1 < argc)
            {
                args.bookmarkPath = argv[++i];
            }
            else if (arg == "--output" && i + 1 < argc)
            {
                args.outputPath = argv[++i];
            }
        }
        args.valid = !args.bookmarkPath.empty();
        return args;
    }

    void PrintTree(const ComponentInfo& node, int depth, ILogger& logger)
    {
        std::string line(static_cast<size_t>(depth) * 2, ' ');
        line += "- " + node.name;
        line += node.visible ? " [visible]" : " [hidden]";
        if (node.suppressed)
        {
            line += " [suppressed]";
        }
        logger.Info(line);

        for (const auto& child : node.children)
        {
            PrintTree(child, depth + 1, logger);
        }
    }

    void CollectComponentNames(const ComponentInfo& node, std::vector<std::string>& names)
    {
        names.push_back(node.name);
        for (const auto& child : node.children)
        {
            CollectComponentNames(child, names);
        }
    }
}

int main(int argc, char** argv)
{
    ConsoleLogger logger;

    CliArgs args = ParseArgs(argc, argv);
    if (!args.valid)
    {
        logger.Error("Usage: CadImportModule.exe --bookmark <path.plmxl> [--output <scene.json>]");
        return 1;
    }

    logger.Info("CAD Import Module starting");

    NxConnector connector(&logger);
    OperationResult<bool> connectResult = connector.Connect(args.bookmarkPath);
    if (!connectResult.success)
    {
        logger.Error("Failed to connect / open bookmark: " + connectResult.errorMessage);
        return 1;
    }

    OperationResult<SessionInfo> sessionInfoResult = connector.GetSessionInfo();
    if (!sessionInfoResult.success)
    {
        logger.Error("Failed to read session info: " + sessionInfoResult.errorMessage);
        return 1;
    }
    const SessionInfo sessionInfo = sessionInfoResult.value;
    logger.Info("Session connected. Model=" + sessionInfo.modelName + " Part=" + sessionInfo.partName);

    NxAssemblyReader assemblyReader(&connector, &logger);
    OperationResult<ComponentInfo> treeResult = assemblyReader.ReadTree();
    if (!treeResult.success)
    {
        logger.Error("Failed to read assembly tree: " + treeResult.errorMessage);
        return 1;
    }
    const ComponentInfo rootComponent = treeResult.value;

    logger.Info("Assembly Tree:");
    PrintTree(rootComponent, 0, logger);

    std::vector<std::string> componentNames;
    CollectComponentNames(rootComponent, componentNames);

    NxGeometryReader geometryReader(&connector, &logger);
    NxMaterialReader materialReader(&connector, &logger);

    std::vector<GeometryInfo> allGeometry;
    std::vector<MaterialInfo> allMaterials;

    for (const std::string& name : componentNames)
    {
        OperationResult<GeometryInfo> geoResult = geometryReader.ReadGeometry(name);
        if (geoResult.success)
        {
            allGeometry.push_back(geoResult.value);
        }
        else
        {
            logger.Warn("Geometry read failed for '" + name + "': " + geoResult.errorMessage);
        }

        OperationResult<std::vector<MaterialInfo>> matResult = materialReader.ReadMaterial(name);
        if (matResult.success)
        {
            allMaterials.insert(allMaterials.end(), matResult.value.begin(), matResult.value.end());
        }
        else
        {
            logger.Warn("Material read failed for '" + name + "': " + matResult.errorMessage);
        }
    }

    // ROI selection is a Placeholder in this phase - no selections are made
    // automatically. ROIManager exists so the future ROI computation
    // module has a stable interface to build on (see IROIManager.h).
    ROIManager roiManager(&logger);

    JsonSceneExporter exporter;
    OperationResult<bool> writeResult = exporter.WriteToFile(
        args.outputPath, sessionInfo, rootComponent, allGeometry, allMaterials, roiManager.GetSelections());

    if (!writeResult.success)
    {
        logger.Error("Failed to write scene manifest: " + writeResult.errorMessage);
        return 1;
    }

    logger.Info("Scene manifest written to: " + args.outputPath);
    logger.Info("CAD Import Module finished");
    return 0;
}

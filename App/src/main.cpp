// CLI entry point that ties CadImportModule and RoiModule together.
//
// This is an External EXE (see CadImportModule/docs/DevGuide.md for why):
// it acquires its OWN NX session via NxConnector, opens the bookmark
// (.plmxl) passed on the command line, reads the Assembly Tree / Geometry
// summary / Material names (CadImportModule), optionally lets the user pick
// ROI Face(s)/Body(ies) (RoiModule), and writes a JSON Scene Manifest that
// downstream modules (Material Engine, Ray Tracing) can consume. Combining
// both modules' output types into one JSON is why JsonSceneExporter lives
// here rather than in either module's Core - see Export/JsonSceneExporter.h.
//
// Usage:
//   CadImportModule.exe --bookmark <path.plmxl> [--output <scene.json>]
//                        [--pick-roi] [--pick-roi-box] [--roi-point x,y,z[:note]]...
//                        [--roi-scope] [--roi-mesh]
//
// All ROI flags are opt-in and independent (can be combined) - see
// RoiModule/Core/include/Core/Interfaces/IRoiResolver.h for what each
// resolves to:
//   --pick-roi        interactive single-Face click, one prompt per Face
//   --pick-roi-box     interactive box/rubber-band drag, many Faces per prompt
//   --roi-point x,y,z  non-interactive: resolves a caller-supplied coordinate
//                       against the live session (repeatable)
//   --roi-scope        interactive box/rubber-band drag at whole-Body
//                       granularity, one independent local scope per prompt
//                       (see RoiModule/README.md "ROI Scope" section) - use
//                       this for "check bottom-corner leakage, then check
//                       back-center leakage" style local analysis passes,
//                       switching NX to a Front/Back view before each drag
//   --roi-mesh         also triangulate each resolved selection's geometry
//                       into ROISelection::mesh (vertices/normals/triangles) -
//                       expensive, so only done when this flag is set
// --pick-roi, --pick-roi-box, and --roi-scope are *blocking* (wait for user
// picks), so they are left out of the default automated run; --roi-point
// never blocks.

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Core/Logging/ConsoleLogger.h"
#include "Core/ROI/ROIManager.h"
#include "Core/ROI/RoiPickingWorkflows.h"
#include "Core/Models/ComponentInfo.h"
#include "Export/JsonSceneExporter.h"

#include "NxBackend/NxConnector.h"
#include "NxBackend/NxAssemblyReader.h"
#include "NxBackend/NxGeometryReader.h"
#include "NxBackend/NxMaterialReader.h"
#include "NxBackend/NxRoiResolver.h"

using namespace CadImport::Core;
using namespace CadImport::NxBackend;
using namespace CadImport::App;

namespace
{
    struct RoiPointArg
    {
        Point3D coordinate;
        std::string note;
    };

    struct CliArgs
    {
        std::string bookmarkPath;
        std::string outputPath = "scene.json";
        bool pickRoi = false;
        bool pickRoiBox = false;
        std::vector<RoiPointArg> roiPoints;
        bool roiScope = false;
        bool roiMesh = false;
        bool valid = false;
    };

    // Parses "x,y,z" or "x,y,z:note" into a RoiPointArg. Returns false (and
    // logs nothing itself - the caller decides how loud to be) on malformed
    // input so one bad --roi-point doesn't take down argument parsing for
    // the rest of the command line.
    bool ParseRoiPointArg(const std::string& raw, RoiPointArg& out)
    {
        std::string numbers = raw;
        const size_t colonPos = raw.find(':');
        if (colonPos != std::string::npos)
        {
            numbers = raw.substr(0, colonPos);
            out.note = raw.substr(colonPos + 1);
        }

        double values[3] = {0.0, 0.0, 0.0};
        size_t start = 0;
        for (int i = 0; i < 3; ++i)
        {
            const size_t commaPos = numbers.find(',', start);
            const std::string token = (i < 2)
                ? numbers.substr(start, commaPos == std::string::npos ? std::string::npos : commaPos - start)
                : numbers.substr(start);

            if (token.empty() || (i < 2 && commaPos == std::string::npos))
            {
                return false;
            }

            try
            {
                values[i] = std::stod(token);
            }
            catch (const std::exception&)
            {
                return false;
            }

            start = commaPos + 1;
        }

        out.coordinate = {values[0], values[1], values[2]};
        return true;
    }

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
            else if (arg == "--pick-roi")
            {
                args.pickRoi = true;
            }
            else if (arg == "--pick-roi-box")
            {
                args.pickRoiBox = true;
            }
            else if (arg == "--roi-point" && i + 1 < argc)
            {
                RoiPointArg pointArg;
                if (ParseRoiPointArg(argv[++i], pointArg))
                {
                    args.roiPoints.push_back(std::move(pointArg));
                }
            }
            else if (arg == "--roi-scope")
            {
                args.roiScope = true;
            }
            else if (arg == "--roi-mesh")
            {
                args.roiMesh = true;
            }
        }
        args.valid = !args.bookmarkPath.empty();
        return args;
    }

    // Below: thin console (y/n prompt + std::cin) loops around the
    // NX-independent orchestration in Core::RoiPickingWorkflows - these
    // loops are the only ROI-picking logic left in App; everything about
    // "what a pick attempt does" lives in Core where it's testable without
    // a live NX session (see RoiModule/tests/CoreTests). All three take
    // IRoiResolver& (the interface), not the concrete NxRoiResolver, for
    // the same reason.

    void RunInteractiveRoiPicking(IRoiResolver& resolver, ROIManager& roiManager, ILogger& logger, bool attachMesh)
    {
        logger.Info("ROI picking: click a Face in the NX graphics window for each prompt.");

        while (true)
        {
            std::cout << "Pick another ROI face? [y/n]: ";
            std::string answer;
            if (!std::getline(std::cin, answer) || (answer != "y" && answer != "Y"))
            {
                break;
            }

            TryAddFaceSelection(resolver, roiManager, "", attachMesh, &logger);
        }
    }

    void RunInteractiveBoxRoiPicking(IRoiResolver& resolver, ROIManager& roiManager, ILogger& logger, bool attachMesh)
    {
        logger.Info("ROI box picking: drag a selection box in the NX graphics window for each prompt.");

        while (true)
        {
            std::cout << "Pick another ROI box? [y/n]: ";
            std::string answer;
            if (!std::getline(std::cin, answer) || (answer != "y" && answer != "Y"))
            {
                break;
            }

            TryAddBoxSelections(resolver, roiManager, "", attachMesh, &logger);
        }
    }

    // One independent local scope per accepted prompt (bottom-corner, then
    // back-center, etc.) - not a running/cumulative selection, and nothing
    // about NX visibility is ever touched here (see RoiModule/README.md for
    // why Hide/Blank was deliberately dropped from this feature: scoping
    // happens at the exported-data level via ROISelection::scopeId, not by
    // mutating what's visible in the graphics window).
    void RunInteractiveRoiScopePicking(IRoiResolver& resolver, ROIManager& roiManager, ILogger& logger, bool attachMesh)
    {
        logger.Info("ROI scope picking: switch NX to a Front or Back view, then drag a box over the local region for each prompt.");

        int scopeCounter = 1;
        while (true)
        {
            std::cout << "Add another ROI scope? [y/n]: ";
            std::string answer;
            if (!std::getline(std::cin, answer) || (answer != "y" && answer != "Y"))
            {
                break;
            }

            std::cout << "Scope label (blank = roi-scope-" << scopeCounter << "): ";
            std::string scopeId;
            std::getline(std::cin, scopeId);
            if (scopeId.empty())
            {
                scopeId = "roi-scope-" + std::to_string(scopeCounter);
            }
            ++scopeCounter;

            int count = TryAddBodySelectionsInBox(resolver, roiManager, scopeId, "", attachMesh, &logger);
            logger.Info("Scope '" + scopeId + "' resolved " + std::to_string(count) + " body(ies).");
        }
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
        logger.Error("Usage: CadImportModule.exe --bookmark <path.plmxl> [--output <scene.json>] [--pick-roi] [--pick-roi-box] [--roi-point x,y,z[:note]] [--roi-scope] [--roi-mesh]");
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

    ROIManager roiManager(&logger);

    const bool anyRoiRequested = args.pickRoi || args.pickRoiBox || args.roiScope || !args.roiPoints.empty();
    if (anyRoiRequested)
    {
        NxRoiResolver roiResolver(&connector, &logger);

        if (args.pickRoi)
        {
            RunInteractiveRoiPicking(roiResolver, roiManager, logger, args.roiMesh);
        }

        if (args.pickRoiBox)
        {
            RunInteractiveBoxRoiPicking(roiResolver, roiManager, logger, args.roiMesh);
        }

        if (args.roiScope)
        {
            RunInteractiveRoiScopePicking(roiResolver, roiManager, logger, args.roiMesh);
        }

        for (const RoiPointArg& pointArg : args.roiPoints)
        {
            TryAddPointSelection(roiResolver, roiManager, pointArg.coordinate, pointArg.note, args.roiMesh, &logger);
        }
    }

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

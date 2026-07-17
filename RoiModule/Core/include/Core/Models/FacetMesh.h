#pragma once

#include <vector>

#include "Core/Models/BoundingBox3D.h"   // Point3D

namespace CadImport::Core
{
    // One facet-mesh vertex: position + its own normal. Per-vertex (not
    // per-triangle) normals let adjacent triangles share a vertex index
    // when the source Face is smooth/curved, same convention as a
    // typical STL/OBJ triangulated export.
    struct MeshVertex
    {
        Point3D position;
        Point3D normal;
    };

    // Indices into FacetMesh::vertices - avoids duplicating shared vertex
    // data across adjacent triangles (faceted output has heavy vertex
    // reuse along shared edges).
    struct MeshTriangle
    {
        int v0 = 0;
        int v1 = 0;
        int v2 = 0;
    };

    // Triangulated surface mesh for one resolved ROI selection - the data
    // Ray Tracing actually needs (vertices + normals + triangle indices),
    // as opposed to ROISelection's own boundingBox/area which are just
    // summary metadata. Extracted on demand
    // (IRoiResolver::ExtractFacetMesh) rather than always populated on
    // pick, since faceting a BLU-sized Face can produce thousands of
    // triangles - too expensive to do unconditionally for every selection.
    struct FacetMesh
    {
        std::vector<MeshVertex> vertices;
        std::vector<MeshTriangle> triangles;
        bool valid = false;
    };
}

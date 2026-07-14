#pragma once

namespace CadImport::Core
{
    struct Point3D
    {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
    };

    // Unit follows SessionInfo::unit (expected: mm)
    struct BoundingBox3D
    {
        Point3D min;
        Point3D max;
        bool valid = false;
    };
}

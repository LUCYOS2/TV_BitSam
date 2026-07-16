#include "NxGeometryUtils.h"

#include <NXOpen/TaggedObject.hxx>
#include <NXOpen/UF/UFSession.hxx>

using namespace CadImport::Core;

namespace NxContracts
{
    BoundingBox3D ComputeBoundingBoxForTag(NXOpen::TaggedObject* object)
    {
        BoundingBox3D box;

        if (object == nullptr)
        {
            return box;
        }

        try
        {
            NXOpen::UF::UFSession* ufSession = NXOpen::UF::UFSession::GetUFSession();
            double range[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            ufSession->Modl.AskBoundingBox(object->Tag(), range);

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

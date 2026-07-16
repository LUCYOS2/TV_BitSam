#pragma once

// Shared by CadImportModule's NxGeometryReader and RoiModule's
// NxRoiResolver - lives in Shared/NxContracts (not either module's own
// NxBackend) since both NX-dependent modules need it equally.

#include "Core/Models/BoundingBox3D.h"

namespace NXOpen
{
    class TaggedObject;
}

namespace NxContracts
{
    // Works for any taggable NX entity (Body, Face, ...) via
    // TaggedObject::Tag() - callers just need a pointer to the specific
    // subtype, since Body/Face/etc. all derive from TaggedObject.
    //
    // TODO(office-PC verify): UF_MODL function name/signature (candidates:
    // AskBoundingBox, AskBoundingBoxExact) against the installed NX2406 SDK.
    CadImport::Core::BoundingBox3D ComputeBoundingBoxForTag(NXOpen::TaggedObject* object);
}

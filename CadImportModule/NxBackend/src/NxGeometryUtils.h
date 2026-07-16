#pragma once

// Internal helper shared by NxGeometryReader and NxRoiResolver - not part of
// NxBackend's public include/ directory, since nothing outside NxBackend's
// own .cpp files needs it.

#include "Core/Models/BoundingBox3D.h"

namespace NXOpen
{
    class TaggedObject;
}

namespace CadImport::NxBackend::detail
{
    // Works for any taggable NX entity (Body, Face, ...) via
    // TaggedObject::Tag() - callers just need a pointer to the specific
    // subtype, since Body/Face/etc. all derive from TaggedObject.
    //
    // TODO(office-PC verify): UF_MODL function name/signature (candidates:
    // AskBoundingBox, AskBoundingBoxExact) against the installed NX2406 SDK.
    Core::BoundingBox3D ComputeBoundingBoxForTag(NXOpen::TaggedObject* object);
}

#pragma once

namespace NXOpen
{
    class Session;
}

namespace NxContracts
{
    // Minimal seam between NX-dependent (NxBackend-level) modules that need
    // to reach the live NXOpen::Session without depending on each other's
    // concrete session-owning class (e.g. RoiModule's NxRoiResolver
    // shouldn't need to know about CadImportModule's NxConnector). Lives
    // outside both modules' Core - it returns an NXOpen type, so it can't
    // go in a NX-independent Core without breaking that boundary.
    class INxSessionAccessor
    {
    public:
        virtual ~INxSessionAccessor() = default;

        virtual NXOpen::Session* RawSession() const = 0;
        virtual bool IsAvailable() const = 0;
    };
}

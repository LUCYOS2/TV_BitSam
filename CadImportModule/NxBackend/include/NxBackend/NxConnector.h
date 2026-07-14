#pragma once

#include <string>

#include "Core/Interfaces/INxSessionProvider.h"
#include "Core/Interfaces/ILogger.h"

// Forward-declared to keep this header lightweight for App-side code that
// only needs the interface pointer type. The .cpp includes the real
// <NXOpen/Session.hxx>.
namespace NXOpen
{
    class Session;
}

namespace CadImport::NxBackend
{
    // Owns this process's NXOpen session.
    //
    // IMPORTANT (confirmed direction, see plan doc): this is an External
    // EXE. Connect() does NOT attach to an already-running interactive NX
    // window - NXOpen::Session::GetSession() is a per-process singleton, so
    // calling it from a standalone process acquires/creates *this
    // process's own* session. After acquiring the session, Connect() opens
    // the part(s) referenced by the given bookmark (.plmxl) file via
    // PartCollection::OpenBaseDisplay (bookmark files are opened the same
    // way as .prt files - see docs/DevGuide.md for source references).
    //
    // NOTE: The exact OpenBaseDisplay overload/parameter list should be
    // confirmed against the installed NX2406 PartCollection.hxx - it has
    // changed across NX versions.
    class NxConnector : public Core::INxSessionProvider
    {
    public:
        explicit NxConnector(Core::ILogger* logger);
        ~NxConnector() override;

        Core::OperationResult<bool> Connect(const std::string& bookmarkPath) override;
        bool IsAvailable() const override;
        Core::OperationResult<Core::SessionInfo> GetSessionInfo() const override;

        // Exposes the raw session pointer for the other NxBackend readers
        // (NxAssemblyReader etc.) to use. Only valid after a successful
        // Connect(). Not part of the Core interface on purpose - Core must
        // never see an NXOpen type.
        NXOpen::Session* RawSession() const;

    private:
        Core::ILogger* m_logger;   // not owned
        NXOpen::Session* m_session = nullptr;
        bool m_connected = false;
    };
}

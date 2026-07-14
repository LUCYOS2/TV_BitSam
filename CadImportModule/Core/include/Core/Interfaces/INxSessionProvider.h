#pragma once

#include <string>

#include "Core/Models/OperationResult.h"
#include "Core/Models/SessionInfo.h"

namespace CadImport::Core
{
    // Owns the lifetime of the NX session used by this process.
    //
    // This module runs as an External EXE: it does NOT attach to an
    // already-running interactive NX window. Connect() creates/acquires this
    // process's own NX session (NXOpen::Session::GetSession()) and then opens
    // the part(s) referenced by the given bookmark (plmxl) file.
    class INxSessionProvider
    {
    public:
        virtual ~INxSessionProvider() = default;

        // Acquires the session and opens the given bookmark file.
        // bookmarkPath: path to a .plmxl bookmark describing which
        // Teamcenter-managed item(s)/revision(s) to load.
        virtual OperationResult<bool> Connect(const std::string& bookmarkPath) = 0;

        virtual bool IsAvailable() const = 0;

        virtual OperationResult<SessionInfo> GetSessionInfo() const = 0;
    };
}

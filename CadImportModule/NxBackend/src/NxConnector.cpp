#include "NxBackend/NxConnector.h"

// NXOpen headers - only ever included from NxBackend .cpp files, never from
// Core or from any header that Core/App would transitively pull in as a
// header-only dependency.
#include <NXOpen/Session.hxx>
#include <NXOpen/PartCollection.hxx>
#include <NXOpen/BasePart.hxx>
#include <NXOpen/NXException.hxx>

using namespace CadImport::Core;

namespace CadImport::NxBackend
{
    NxConnector::NxConnector(Core::ILogger* logger) : m_logger(logger)
    {
    }

    NxConnector::~NxConnector() = default;

    OperationResult<bool> NxConnector::Connect(const std::string& bookmarkPath)
    {
        try
        {
            // External application: this acquires *this process's own*
            // session, it does not reach into an already-open NX window.
            m_session = NXOpen::Session::GetSession();

            if (m_logger)
            {
                m_logger->Info("NX session acquired, opening bookmark: " + bookmarkPath);
            }

            // TODO(office-PC verify): confirm the exact OpenBaseDisplay
            // overload against NX2406's PartCollection.hxx. As of several
            // NX versions this call also accepts bookmark (.plmxl) paths in
            // addition to .prt files - see docs/DevGuide.md references.
            NXOpen::PartLoadStatus* loadStatus = nullptr;
            NXOpen::BasePart* part = m_session->Parts()->OpenBaseDisplay(bookmarkPath, &loadStatus);

            if (loadStatus != nullptr)
            {
                loadStatus->Dispose();
            }

            if (part == nullptr)
            {
                m_connected = false;
                const std::string msg = "OpenBaseDisplay returned null for bookmark: " + bookmarkPath;
                if (m_logger) m_logger->Error(msg);
                return OperationResult<bool>::Fail(msg);
            }

            m_connected = true;
            if (m_logger)
            {
                m_logger->Info("Bookmark opened successfully: " + bookmarkPath);
            }
            return OperationResult<bool>::Ok(true);
        }
        catch (const NXOpen::NXException& ex)
        {
            m_connected = false;
            const std::string msg = std::string("NXException while connecting: ") + ex.Message();
            if (m_logger) m_logger->Error(msg);
            return OperationResult<bool>::Fail(msg);
        }
        catch (const std::exception& ex)
        {
            m_connected = false;
            const std::string msg = std::string("Unexpected error while connecting: ") + ex.what();
            if (m_logger) m_logger->Error(msg);
            return OperationResult<bool>::Fail(msg);
        }
    }

    bool NxConnector::IsAvailable() const
    {
        return m_connected && m_session != nullptr;
    }

    OperationResult<SessionInfo> NxConnector::GetSessionInfo() const
    {
        if (!IsAvailable())
        {
            return OperationResult<SessionInfo>::Fail("Session not connected - call Connect() first");
        }

        try
        {
            SessionInfo info;
            NXOpen::BasePart* workPart = m_session->Parts()->Work();
            if (workPart == nullptr)
            {
                return OperationResult<SessionInfo>::Fail("No Work Part is currently loaded");
            }

            // TODO(office-PC verify): exact accessor names for Revision /
            // Unit against NX2406 BasePart.hxx (unit is commonly read via
            // PartUnits on Part, not BasePart - narrow this down once the
            // real headers are available).
            info.partName = workPart->Name();
            info.modelName = workPart->Name();
            info.connected = true;

            return OperationResult<SessionInfo>::Ok(info);
        }
        catch (const NXOpen::NXException& ex)
        {
            return OperationResult<SessionInfo>::Fail(std::string("NXException while reading session info: ") + ex.Message());
        }
    }

    NXOpen::Session* NxConnector::RawSession() const
    {
        return m_session;
    }
}

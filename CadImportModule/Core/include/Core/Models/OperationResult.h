#pragma once

#include <string>
#include <utility>

namespace CadImport::Core
{
    // Generic success/failure envelope. NxBackend catches NXOpen::NXException
    // (and any other SDK-specific error type) internally and translates it
    // into this type, so Core and App never need to know about NXOpen.
    template <typename T>
    struct OperationResult
    {
        bool success = false;
        std::string errorMessage;
        T value{};

        static OperationResult<T> Ok(T val)
        {
            OperationResult<T> r;
            r.success = true;
            r.value = std::move(val);
            return r;
        }

        static OperationResult<T> Fail(std::string message)
        {
            OperationResult<T> r;
            r.success = false;
            r.errorMessage = std::move(message);
            return r;
        }
    };
}

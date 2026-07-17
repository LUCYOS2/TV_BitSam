#pragma once

// Header-only test harness - no external dependency (Catch2 etc. not
// vendored in this repo yet). Enough for RoiModule::Core's needs: named
// test cases, CHECK() that keeps running after a failed assertion, and a
// process exit code that reflects pass/fail (0 = all green).

#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace MiniTest
{
    struct Registry
    {
        static std::vector<std::pair<std::string, std::function<void()>>>& Cases()
        {
            static std::vector<std::pair<std::string, std::function<void()>>> cases;
            return cases;
        }

        static int& Failures()
        {
            static int failures = 0;
            return failures;
        }
    };

    struct Registrar
    {
        Registrar(const std::string& name, std::function<void()> fn)
        {
            Registry::Cases().emplace_back(name, std::move(fn));
        }
    };

    inline void ReportFailure(const std::string& file, int line, const std::string& expr)
    {
        ++Registry::Failures();
        std::cerr << "    [FAIL] " << file << ":" << line << "  CHECK(" << expr << ")\n";
    }

    inline int RunAll()
    {
        const int total = static_cast<int>(Registry::Cases().size());
        std::cout << "Running " << total << " test case(s)...\n";

        for (auto& [name, fn] : Registry::Cases())
        {
            const int failuresBefore = Registry::Failures();
            std::cout << "- " << name << "\n";
            fn();
            if (Registry::Failures() == failuresBefore)
            {
                std::cout << "    [OK]\n";
            }
        }

        if (Registry::Failures() == 0)
        {
            std::cout << "\nAll tests passed (" << total << ").\n";
            return 0;
        }

        std::cout << "\n" << Registry::Failures() << " check(s) failed.\n";
        return 1;
    }
}

#define TEST_CASE(name)                                                          \
    void name();                                                                 \
    static MiniTest::Registrar registrar_##name(#name, name);                    \
    void name()

#define CHECK(expr)                                                              \
    do                                                                           \
    {                                                                            \
        if (!(expr)) MiniTest::ReportFailure(__FILE__, __LINE__, #expr);         \
    } while (0)

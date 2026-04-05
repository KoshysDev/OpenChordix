#include "AppPaths.h"

#include <system_error>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace
{
    std::filesystem::path resolveExecutablePath()
    {
#if defined(_WIN32)
        std::vector<wchar_t> buffer(260, L'\0');
        while (true)
        {
            const DWORD copied = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
            if (copied == 0)
            {
                break;
            }

            if (copied < buffer.size() - 1)
            {
                return std::filesystem::path(std::wstring(buffer.data(), copied));
            }

            buffer.resize(buffer.size() * 2U, L'\0');
        }
#elif defined(__APPLE__)
        uint32_t pathSize = 0;
        _NSGetExecutablePath(nullptr, &pathSize);
        if (pathSize > 0)
        {
            std::vector<char> buffer(pathSize, '\0');
            if (_NSGetExecutablePath(buffer.data(), &pathSize) == 0)
            {
                std::error_code ec;
                const auto canonicalPath = std::filesystem::weakly_canonical(buffer.data(), ec);
                if (!ec)
                {
                    return canonicalPath;
                }
                return std::filesystem::path(buffer.data());
            }
        }
#elif defined(__linux__)
        std::error_code ec;
        const auto symlinkPath = std::filesystem::read_symlink("/proc/self/exe", ec);
        if (!ec)
        {
            return symlinkPath;
        }
#endif

        return std::filesystem::current_path();
    }
}

namespace openchordix::core
{
    std::filesystem::path executableDirectory()
    {
        static const std::filesystem::path cachedDir = resolveExecutablePath().parent_path();
        return cachedDir;
    }
}


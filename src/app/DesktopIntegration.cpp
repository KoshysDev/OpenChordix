#include "DesktopIntegration.h"

#if defined(__linux__)
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include "EmbeddedAssets.h"

namespace
{
    std::filesystem::path executablePath()
    {
        std::error_code ec;
        auto exePath = std::filesystem::read_symlink("/proc/self/exe", ec);
        if (ec)
        {
            return {};
        }
        return exePath;
    }

    void writeFileIfMissing(const std::filesystem::path &path, const unsigned char *data, std::size_t size)
    {
        if (!data || size == 0)
        {
            return;
        }

        std::error_code ec;
        if (std::filesystem::exists(path, ec))
        {
            return;
        }

        std::ofstream out(path, std::ios::binary);
        if (!out)
        {
            return;
        }
        out.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(size));
    }

    void writeFileOverwrite(const std::filesystem::path &path, const std::string &content)
    {
        std::ofstream out(path);
        if (!out)
        {
            return;
        }
        out << content;
    }
}

namespace openchordix::app
{
    void ensureLocalDesktopEntry()
    {
        const auto exePath = executablePath();
        if (exePath.empty())
        {
            return;
        }

        const auto launchDir = std::filesystem::current_path();
        if (launchDir.empty())
        {
            return;
        }

        const auto assetsDir = launchDir / "assets";

        std::error_code ec;
        if (!std::filesystem::exists(assetsDir, ec))
        {
            std::filesystem::create_directories(assetsDir, ec);
            if (ec)
            {
                return;
            }

            for (const auto &asset : openchordix::assets::listEmbeddedAssets())
            {
                const auto outPath = assetsDir / asset.name;
                if (!outPath.parent_path().empty())
                {
                    std::filesystem::create_directories(outPath.parent_path(), ec);
                    if (ec)
                    {
                        continue;
                    }
                }
                writeFileIfMissing(outPath, asset.data, asset.size);
            }
        }

        const auto desktopPath = launchDir / "OpenChordix.desktop";
        const auto iconPath = assetsDir / "icons" / "AppIcon.png";

        std::string content;
        content += "[Desktop Entry]\n";
        content += "Name=OpenChordix\n";
        content += "Comment=OpenChordix Pitch Detection\n";
        content += "Exec=" + exePath.string() + "\n";
        content += "Icon=" + iconPath.string() + "\n";
        content += "Terminal=false\n";
        content += "Type=Application\n";
        content += "Categories=AudioVideo;Audio;\n";
        content += "StartupWMClass=OpenChordix\n";
        writeFileOverwrite(desktopPath, content);

        std::filesystem::permissions(desktopPath,
                                     std::filesystem::perms::owner_exec |
                                         std::filesystem::perms::group_exec |
                                         std::filesystem::perms::others_exec,
                                     std::filesystem::perm_options::add,
                                     ec);
    }
}
#else
namespace openchordix::app
{
    void ensureLocalDesktopEntry() {}
}
#endif

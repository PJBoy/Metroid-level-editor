#include "global.h"

std::filesystem::path DebugFile::dataDirectory;

struct FilepathHash
{
    size_t operator()(const std::filesystem::path& filepath) const
    {
        return hash_value(weakly_canonical(filepath));
    }
};

DebugFile::DebugFile(std::filesystem::path filename) noexcept
{
    try
    {
        const std::filesystem::path filepath(dataDirectory / filename);

        // If first use of file, clear it, otherwise insert spaces
        static std::unordered_set<std::filesystem::path, FilepathHash> initialised;
        if (initialised.insert(filepath).second)
            open(filepath, trunc);
        else
            open(filepath, app | ate);

        const std::time_t time(std::time({}));
        *this << std::put_time(std::gmtime(&time), "%c") << " - "s;
    }
    catch (const std::exception&)
    {
        // There's nothing I can do to handle this exception; but catch it anyways, because this is a non-fatal error
    }
}

void DebugFile::init(std::filesystem::path dataDirectory_in) noexcept
{
    dataDirectory = std::move(dataDirectory_in);
}

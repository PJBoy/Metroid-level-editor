module;

#include "global.h"

export module os;

export import config;
// import main_window; // window module et al imports os module, so this is not allowed

export struct FileFilter
{
    std::string_view label, glob;
};

export class Os
{
protected:
    Config* p_config{};

public:
    // Set up general OS state
    Os() = default;

    Os(const Os&) = delete;
    auto operator=(Os) = delete;
    virtual ~Os() = default;

    // Requires configured state
    virtual int eventLoop() = 0;

    // Does not require configured state
    virtual std::filesystem::path getDataDirectory() const = 0;
    virtual void error(const std::string& errorText) const = 0;

    // Set up config influenced state
    virtual void init(Config& config)
    {
        p_config = &config;
    }

    virtual void spawnMainWindow(class MainWindow& window, std::string_view className, std::string_view title, std::any arg) = 0;
    virtual void quit() = 0;
    virtual std::optional<std::filesystem::path> chooseFile(std::span<const FileFilter> fileFilters, FunctionRef<bool(const std::filesystem::path&)> validator) const = 0;
};

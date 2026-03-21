module;

#define OEMRESOURCE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>

#include "../global.h"

export module os_windows;

export import os;

export import main_window;
export import window;


export class WindowsError : public std::runtime_error
{
    // GetLastError reference: https://learn.microsoft.com/en-gb/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror

protected:
    static std::wstring getErrorMessage(unsigned long errorId);
    static std::string makeMessage() noexcept;
    static std::string makeMessage(const std::string& extraMessage) noexcept;
    static std::string makeMessage(unsigned long errorId) noexcept;
    static std::string makeMessage(unsigned long errorId, const std::string& extraMessage) noexcept;

public:
    WindowsError() noexcept;
    WindowsError(unsigned long errorId) noexcept;
    WindowsError(const std::string& extraMessage) noexcept;
    WindowsError(unsigned long errorId, const std::string& extraMessage) noexcept;
};

export class CommonDialogError : public std::runtime_error
{
    // CommDlgExtendedError reference: https://learn.microsoft.com/en-gb/windows/win32/api/commdlg/nf-commdlg-commdlgextendederror

protected:
    static std::string getErrorMessage(unsigned long errorId);
    static std::string makeMessage() noexcept;
    static std::string makeMessage(const std::string& extraMessage) noexcept;
    static std::string makeMessage(unsigned long errorId) noexcept;
    static std::string makeMessage(unsigned long errorId, const std::string& extraMessage) noexcept;

public:
    CommonDialogError() noexcept;
    CommonDialogError(unsigned long errorId) noexcept;
    CommonDialogError(const std::string& extraMessage) noexcept;
    CommonDialogError(unsigned long errorId, const std::string& extraMessage) noexcept;
};


export class Windows final : public Os
{
public:
    using MainWindowArg_t = int;

private:
    HINSTANCE instance;

public:
    Windows(HINSTANCE instance) noexcept;
    ~Windows() override = default;

    void init(Config& config) override;
    int eventLoop() override;
    std::filesystem::path getDataDirectory() const override;
    void error(const std::string& errorText) const override;
    void spawnMainWindow(MainWindow& window, std::string_view className, std::string_view title, std::any arg) override;
    void quit() override;
};

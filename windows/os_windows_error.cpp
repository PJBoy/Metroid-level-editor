#include "../global.h"

#define OEMRESOURCE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cderr.h>
#include <commctrl.h>
#include <commdlg.h>
#include <windowsx.h>

import os_windows;

import string;


// WindowError
std::wstring WindowsError::getErrorMessage(unsigned long errorId)
try
{
    // FormatMessage reference: https://learn.microsoft.com/en-gb/windows/win32/api/winbase/nf-winbase-formatmessage
    const unsigned long flags(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS);
    const void* const source{};
    const unsigned long languageId(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
    wchar_t* formattedMessage;
    const unsigned long size{};
    if (!FormatMessage(flags, source, errorId, languageId, reinterpret_cast<wchar_t*>(&formattedMessage), size, nullptr))
        return L"Unknown error. FormatMessage failed with error code " + std::to_wstring(GetLastError());
        
    std::wstring ret(formattedMessage);
    LocalFree(formattedMessage);
    return ret;
}
LOG_RETHROW

std::string WindowsError::makeMessage() noexcept
{
    // TODO: add stack trace
    // https://msdn.microsoft.com/en-us/library/windows/desktop/bb204633
    return makeMessage(GetLastError());
}

std::string WindowsError::makeMessage(const std::string& extraMessage) noexcept
{
    return makeMessage(GetLastError(), extraMessage);
}

std::string WindowsError::makeMessage(unsigned long errorId) noexcept
try
{
    const std::string errorMessage(toString(getErrorMessage(errorId)));
    return "Win32 API error occurred, error code " + std::to_string(errorId) + ": " + errorMessage;
}
catch (...)
{
    return "Win32 API error occurred. Unknown error. An exception was thrown during error message construction.";
}

std::string WindowsError::makeMessage(unsigned long errorId, const std::string& extraMessage) noexcept
try
{
    return makeMessage(errorId) + ' ' + extraMessage;
}
catch (...)
{
    return "Win32 API error occurred. Unknown error. An exception was thrown during error message construction.";
}

WindowsError::WindowsError() noexcept
    : std::runtime_error(makeMessage())
{}

WindowsError::WindowsError(unsigned long errorId) noexcept
    : std::runtime_error(makeMessage(errorId))
{}

WindowsError::WindowsError(const std::string& extraMessage) noexcept
    : std::runtime_error(makeMessage(extraMessage))
{}

WindowsError::WindowsError(unsigned long errorId, const std::string& extraMessage) noexcept
    : std::runtime_error(makeMessage(errorId, extraMessage))
{}


// CommonDialogError
std::string CommonDialogError::getErrorMessage(unsigned long errorId)
try
{
    switch (errorId)
    {
    default:
        return "Unknown error."s;

    case CDERR_DIALOGFAILURE:
        return "CDERR_DIALOGFAILURE: The dialog box could not be created. The common dialog box function's call to the DialogBox function failed. For example, this error occurs if the common dialog box call specifies an invalid window handle."s;

    case CDERR_FINDRESFAILURE:
        return "CDERR_FINDRESFAILURE: The common dialog box function failed to find a specified resource."s;

    case CDERR_INITIALIZATION:
        return "CDERR_INITIALIZATION: The common dialog box function failed during initialization. This error often occurs when sufficient memory is not available."s;

    case CDERR_LOADRESFAILURE:
        return "CDERR_LOADRESFAILURE: The common dialog box function failed to load a specified resource."s;

    case CDERR_LOADSTRFAILURE:
        return "CDERR_LOADSTRFAILURE: The common dialog box function failed to load a specified string."s;

    case CDERR_LOCKRESFAILURE:
        return "CDERR_LOCKRESFAILURE: The common dialog box function failed to lock a specified resource."s;

    case CDERR_MEMALLOCFAILURE:
        return "CDERR_MEMALLOCFAILURE: The common dialog box function was unable to allocate memory for internal structures."s;

    case CDERR_MEMLOCKFAILURE:
        return "CDERR_MEMLOCKFAILURE: The common dialog box function was unable to lock the memory associated with a handle."s;

    case CDERR_NOHINSTANCE:
        return "CDERR_NOHINSTANCE: The ENABLETEMPLATE flag was set in the Flags member of the initialization structure for the corresponding common dialog box, but you failed to provide a corresponding instance handle."s;

    case CDERR_NOHOOK: 
        return "CDERR_NOHOOK: The ENABLEHOOK flag was set in the Flags member of the initialization structure for the corresponding common dialog box, but you failed to provide a pointer to a corresponding hook procedure."s;

    case CDERR_NOTEMPLATE:
        return "CDERR_NOTEMPLATE: The ENABLETEMPLATE flag was set in the Flags member of the initialization structure for the corresponding common dialog box, but you failed to provide a corresponding template."s;

    case CDERR_REGISTERMSGFAIL:
        return "CDERR_REGISTERMSGFAIL: The RegisterWindowMessage function returned an error code when it was called by the common dialog box function."s;

    case CDERR_STRUCTSIZE:
        return "CDERR_STRUCTSIZE: The lStructSize member of the initialization structure for the corresponding common dialog box is invalid."s;

    case CFERR_MAXLESSTHANMIN:
        return "CFERR_MAXLESSTHANMIN: The size specified in the nSizeMax member of the CHOOSEFONT structure is less than the size specified in the nSizeMin member."s;

    case CFERR_NOFONTS:
        return "CFERR_NOFONTS: No fonts exist."s;

    case FNERR_BUFFERTOOSMALL:
        return "FNERR_BUFFERTOOSMALL: The buffer pointed to by the lpstrFile member of the OPENFILENAME structure is too small for the file name specified by the user. The first two bytes of the lpstrFile buffer contain an integer value specifying the size required to receive the full name, in characters."s;

    case FNERR_INVALIDFILENAME:
        return "FNERR_INVALIDFILENAME: A file name is invalid."s;

    case FNERR_SUBCLASSFAILURE:
        return "FNERR_SUBCLASSFAILURE: An attempt to subclass a list box failed because sufficient memory was not available."s;

    case FRERR_BUFFERLENGTHZERO:
        return "FRERR_BUFFERLENGTHZERO: A member of the FINDREPLACE structure points to an invalid buffer."s;
    }
}
LOG_RETHROW

std::string CommonDialogError::makeMessage() noexcept
{
    return makeMessage(CommDlgExtendedError());
}

std::string CommonDialogError::makeMessage(const std::string& extraMessage) noexcept
{
    return makeMessage(CommDlgExtendedError(), extraMessage);
}

std::string CommonDialogError::makeMessage(unsigned long errorId) noexcept
try
{
    const std::string errorMessage(getErrorMessage(errorId));
    return "Win32 API common dialog box error occurred, error code " + std::to_string(errorId) + ": " + errorMessage;
}
catch (...)
{
    return "Win32 API common dialog box error occurred. Unknown error. An exception was thrown during error message construction.";
}

std::string CommonDialogError::makeMessage(unsigned long errorId, const std::string& extraMessage) noexcept
try
{
    return makeMessage(errorId) + ' ' + extraMessage;
}
catch (...)
{
    return "Win32 API common dialog box error occurred. Unknown error. An exception was thrown during error message construction.";
}

CommonDialogError::CommonDialogError() noexcept
    : std::runtime_error(makeMessage())
{}

CommonDialogError::CommonDialogError(unsigned long errorId) noexcept
    : std::runtime_error(makeMessage(errorId))
{}

CommonDialogError::CommonDialogError(const std::string& extraMessage) noexcept
    : std::runtime_error(makeMessage(extraMessage))
{}

CommonDialogError::CommonDialogError(unsigned long errorId, const std::string& extraMessage) noexcept
    : std::runtime_error(makeMessage(errorId, extraMessage))
{}


// ComError
std::string ComError::makeMessage(long result) noexcept
try
{
    const unsigned facility = HRESULT_FACILITY(result);
    const unsigned code = HRESULT_CODE(result);

    std::string ret = "COM error occurred. Facility 0x";
    ret += std::to_string(facility);
    ret += " (";
    switch (facility)
    {
    default:
        ret += "unknown";
        break;

    #define CASE(code) \
    case code: \
        ret += #code; \
        break; \

        CASE(FACILITY_NULL)
        CASE(FACILITY_RPC)
        CASE(FACILITY_DISPATCH)
        CASE(FACILITY_STORAGE)
        CASE(FACILITY_ITF)
        CASE(FACILITY_WIN32)
        CASE(FACILITY_WINDOWS)
        CASE(FACILITY_SECURITY) // AKA FACILITY_SSPI
        CASE(FACILITY_CONTROL)
        CASE(FACILITY_CERT)
        CASE(FACILITY_INTERNET)
        CASE(FACILITY_MEDIASERVER)
        CASE(FACILITY_MSMQ)
        CASE(FACILITY_SETUPAPI)
        CASE(FACILITY_SCARD)
        CASE(FACILITY_COMPLUS)
        CASE(FACILITY_AAF)
        CASE(FACILITY_URT)
        CASE(FACILITY_ACS)
        CASE(FACILITY_DPLAY)
        CASE(FACILITY_UMI)
        CASE(FACILITY_SXS)
        CASE(FACILITY_WINDOWS_CE)
        CASE(FACILITY_HTTP)
        CASE(FACILITY_USERMODE_COMMONLOG)
        CASE(FACILITY_USERMODE_FILTER_MANAGER)
        CASE(FACILITY_BACKGROUNDCOPY)
        CASE(FACILITY_CONFIGURATION)
        CASE(FACILITY_STATE_MANAGEMENT)
        CASE(FACILITY_METADIRECTORY)
        CASE(FACILITY_WINDOWSUPDATE)
        CASE(FACILITY_DIRECTORYSERVICE)
        CASE(FACILITY_GRAPHICS)
        CASE(FACILITY_SHELL)
        CASE(FACILITY_TPM_SERVICES)
        CASE(FACILITY_TPM_SOFTWARE)
        CASE(FACILITY_PLA)
        CASE(FACILITY_FVE)
        CASE(FACILITY_FWP)
        CASE(FACILITY_WINRM)
        CASE(FACILITY_NDIS)
        CASE(FACILITY_USERMODE_HYPERVISOR)
        CASE(FACILITY_CMI)
        CASE(FACILITY_USERMODE_VIRTUALIZATION)
        CASE(FACILITY_USERMODE_VOLMGR)
        CASE(FACILITY_BCD)
        CASE(FACILITY_USERMODE_VHD)
        CASE(FACILITY_SDIAG)
        CASE(FACILITY_WEBSERVICES)
        CASE(FACILITY_WINDOWS_DEFENDER)
        CASE(FACILITY_OPC)
    #undef CASE
    }

    ret += ") - code ";
    ret += std::to_string(code);

    if (facility == FACILITY_WIN32)
    {
        ret += ": ";
        ret += WindowsError(code).what();
    }

    return ret;
}
catch (const std::exception&)
{
    return "COM error occurred. Unknown error. An exception was thrown during error message construction.";
}

std::string ComError::makeMessage(long result, const std::string& extraMessage) noexcept
try
{
    return makeMessage(result) + ' ' + extraMessage;
}
catch (const std::exception&)
{
    return "COM error occurred. Unknown error. An exception was thrown during error message construction.";
}

ComError::ComError(long result) noexcept
    : std::runtime_error(makeMessage(result))
{}

ComError::ComError(long result, const std::string& extraMessage) noexcept
    : std::runtime_error(makeMessage(result, extraMessage))
{}

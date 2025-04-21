

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>
/// only here for testing. Source: https://stackoverflow.com/a/17387176
//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString()
{
    //Get the error message ID, if any.
    DWORD errorMessageID = ::GetLastError();
    if(errorMessageID == 0) {
        return std::string(); //No error message has been recorded
    }

    LPSTR messageBuffer = nullptr;

    //Ask Win32 to give us the string version of that message ID.
    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    //Copy the error message into a std::string.
    std::string message(messageBuffer, size);

    //Free the Win32's string's buffer.
    LocalFree(messageBuffer);

    return message;
}
/// /source

void set_component_native_owning_window(juce::Component& to_be_owned, juce::Component& to_be_owner) {
    SetLastError(0); /// windows docs say to do SetLastError(0) before calling SetWindowLongPtr
    /// I know this says GWLP_HWNDPARENT (emphasis on the PARENT), but I promise you this sets the window OWNER, not the window parent
    /// I have no idea why microsoft decided to do this
    /// source: https://stackoverflow.com/a/133415
    /// source: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowlongptra#return-value:~:text=Do%20not%20call%20SetWindowLongPtr%20with%20the%20GWLP_HWNDPARENT%20index%20to%20change%20the%20parent%20of%20a%20child%20window.%20Instead%2C%20use%20the%20SetParent%20function.
    if(  !SetWindowLongPtr(reinterpret_cast<HWND>(to_be_owned.getWindowHandle()), GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(to_be_owner.getWindowHandle()))
       && GetLastError()) { // failure is indicated by SetWindowLongPtr() returning null AND GetLastError() returning nonzero  https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowlongptra#return-value:~:text=If%20the%20previous,that%20is%20nonzero.

        to_be_owned.setAlwaysOnTop(true); // fallback behavior. Not ideal but close enough to what we want
    }
    else { // success
        // windows api docs say I should call SetWindowPos here to force the window to update any cached data it might have https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowlongptra#:~:text=Certain%20window%20data%20is%20cached%2C%20so%20changes%20you%20make%20using%20SetWindowLongPtr%20will%20not%20take%20effect%20until%20you%20call%20the%20SetWindowPos%20function.
        // but it seems to work fine without doing that
    }
}
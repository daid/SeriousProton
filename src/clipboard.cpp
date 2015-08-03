#include "engine.h"

#ifdef __WIN32__
#include <windows.h>
#endif

string Clipboard::readClipboard()
{
#ifdef __WIN32__
    P<WindowManager> windowManager = engine->getObject("windowManager");
    if (!OpenClipboard(windowManager->window.getSystemHandle()))
        return "";
    HANDLE handle = GetClipboardData(CF_TEXT);
    if (!handle)
    {
        CloseClipboard();
        return "";
    }
    string ret;
    ret = static_cast<char*>(GlobalLock(handle));
    GlobalUnlock(handle);
    CloseClipboard();
    return ret;
#endif//__WIN32__
    return "";
}

void Clipboard::setClipboard(string value)
{
#ifdef __WIN32__
    P<WindowManager> windowManager = engine->getObject("windowManager");
    if (!OpenClipboard(windowManager->window.getSystemHandle()))
        return;

    EmptyClipboard();

    HANDLE string_handle;
    size_t string_size = (value.length()+1) * sizeof(char);
    string_handle = GlobalAlloc(GMEM_MOVEABLE, string_size);

    if (!string_handle)
    {
        CloseClipboard();
        return;
    }

    memcpy(GlobalLock(string_handle), value.c_str(), string_size);
    GlobalUnlock(string_handle);
    SetClipboardData(CF_TEXT, string_handle);

    CloseClipboard();
#endif//__WIN32__
}

#include "engine.h"

#ifdef __WIN32__
#include <windows.h>
#endif
#ifdef __linux__
#include <stdio.h>
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
#ifdef __linux__
    FILE* pipe = popen("/usr/bin/xclip -o -selection clipboard", "r");
    if (!pipe)
    {
        LOG(WARNING) << "Failed to execute /usr/bin/xclip for clipboard access");
        return "";
    }
    char buffer[1024];
    std::string result = "";
    while (!feof(pipe))
    {
        if (fgets(buffer, 1024, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
#endif
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
#ifdef __linux__
    FILE* pipe = popen("/usr/bin/xclip -i -selection clipboard -silent", "we");
    if (!pipe)
    {
        LOG(WARNING) << "Failed to execute /usr/bin/xclip for clipboard access");
        return;
    }
    fwrite(value.c_str(), value.size(), 1, pipe);
    pclose(pipe);
#endif
}

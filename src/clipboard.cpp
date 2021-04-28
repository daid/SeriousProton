#include "engine.h"

#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __linux__
#include <stdio.h>
#endif

string Clipboard::readClipboard()
{
#ifdef _WIN32
    P<WindowManager> windowManager = engine->getObject("windowManager");
    if (!OpenClipboard(windowManager->window.getSystemHandle()))
    {
        LOG(WARNING) << "Failed to open the clipboard for reading";
        return "";
    }
    HANDLE handle = GetClipboardData(CF_TEXT);
    if (!handle)
    {
        LOG(WARNING) << "Failed to open the clipboard for reading";
        CloseClipboard();
        return "";
    }
    string ret;
    ret = static_cast<char*>(GlobalLock(handle));
    GlobalUnlock(handle);
    CloseClipboard();
    return ret;
#endif//_WIN32
#if defined(__linux__) || defined(__APPLE__)
#ifdef __APPLE__
    const char* cmd = "/usr/bin/pbpaste 2>&1";
#else
    const char* cmd = "/usr/bin/xclip -o -selection clipboard";
#endif
    FILE* pipe = popen(cmd, "r");

    if (!pipe)
    {
        LOG(WARNING) << "Failed to execute " << cmd << " for clipboard access";
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
    return result;
#endif//__linux__ || __APPLE__
}

void Clipboard::setClipboard(string value)
{
#ifdef _WIN32
    P<WindowManager> windowManager = engine->getObject("windowManager");
    if (!OpenClipboard(windowManager->window.getSystemHandle()))
    {
        LOG(WARNING) << "Failed to open the clipboard for writing";
        return;
    }

    EmptyClipboard();

    HANDLE string_handle;
    size_t string_size = (value.length()+1) * sizeof(char);
    string_handle = GlobalAlloc(GMEM_MOVEABLE, string_size);

    if (!string_handle)
    {
        LOG(WARNING) << "Failed to allocate a string for the clipboard writing";
        CloseClipboard();
        return;
    }

    memcpy(GlobalLock(string_handle), value.c_str(), string_size);
    GlobalUnlock(string_handle);
    SetClipboardData(CF_TEXT, string_handle);

    CloseClipboard();
#endif//_WIN32
#if defined(__linux__) || defined(__APPLE__)
#ifdef __APPLE__
    const char* cmd = "/usr/bin/pbcopy > /dev/null 2>&1";
    const char* mode = "w";
#else
    const char* cmd = "/usr/bin/xclip -i -selection clipboard -silent";
    const char* mode = "we";
#endif
    FILE* pipe = popen(cmd, mode);

    if (!pipe)
    {
        LOG(WARNING) << "Failed to execute " << cmd << " for clipboard access";
        return;
    }

    fwrite(value.c_str(), value.size(), 1, pipe);
    pclose(pipe);
#endif//__linux__ || __APPLE__
}

#include "engine.h"
#include <SDL.h>


string Clipboard::readClipboard()
{
    auto str = SDL_GetClipboardText();
    string result = str;
    SDL_free(str);
    return result;
}

void Clipboard::setClipboard(string value)
{
    SDL_SetClipboardText(value.c_str());
}

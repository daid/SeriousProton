#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include "stringImproved.h"

class Clipboard
{
public:
    static string readClipboard();
    static void setClipboard(string value);
};

#endif//CLIPBOARD_H

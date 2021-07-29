#include <stringutil/base64.h>
#include <logging.h>

#include <cstring>

namespace sp {
namespace stringutil {
namespace base64 {

static unsigned char encode_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static unsigned char decode_table[] = {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  0,  0, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,
     0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0,  0,
     0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

string encode(const string& data)
{
    string result;
    result.reserve(((data.length() + 2) / 3) * 4);
    for(unsigned int n=0; n<data.length(); n+=3)
    {
        uint8_t d[3];
        d[0] = data[n];
        if (n + 1 < data.length())
            d[1] = data[n+1];
        else
            d[1] = 0;
        if (n + 2 < data.length())
            d[2] = data[n+2];
        else
            d[2] = 0;
        
        result += encode_table[d[0] >> 2];
        result += encode_table[((d[0] & 0x03) << 4) | (d[1] >> 4)];
        if (n + 1 < data.length())
            result += encode_table[((d[1] & 0x0f) << 2) | (d[2] >> 6)];
        else
            result += '=';
        if (n + 2 < data.length())
            result += encode_table[d[2] & 0x3f];
        else
            result += '=';
    }
    
    return result;
}

string decode(const string& data)
{
    string result;
    result.reserve(data.length() / 4 * 3 + 2);

    for(unsigned int n=0; n<data.length(); n+=4)
    {
        uint8_t c[4];
        c[0] = decode_table[uint8_t(data[n])];
        if (n + 1 < data.length())
            c[1] = decode_table[uint8_t(data[n + 1])];
        else
            c[1] = 0;
        if (n + 2 < data.length())
            c[2] = decode_table[uint8_t(data[n + 2])];
        else
            c[2] = 0;
        if (n + 3 < data.length())
            c[3] = decode_table[uint8_t(data[n + 3])];
        else
            c[3] = 0;
        
        result += (c[0] << 2) | (c[1] >> 4);
        if (n + 2 < data.length() && data[n + 2] != '=')
            result += (c[1] << 4) | (c[2] >> 2);
        if (n + 3 < data.length() && data[n + 3] != '=')
            result += (c[2] << 6) | (c[3]);
    }
    
    return result;
}

}//namespace base64
}//namespace stringutil
}//namespace sp

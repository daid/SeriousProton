#include <stringutil/sha1.h>
#include <stringutil/base64.h>
#include <logging.h>

static inline uint32_t rotate_left(uint32_t value, unsigned int bits)
{
  return (value << bits) | (value >> (sizeof(value)*8 - bits));
}

namespace sp {
namespace stringutil {

SHA1::SHA1(string input)
{
    //Initialize variables:
    uint32_t h[5] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};

    //message length in bits (always a multiple of the number of bits in a character).
    uint64_t ml = input.size() * 8;

    //Pre-processing:
    //append the bit '1' to the message e.g. by adding 0x80 if message length is a multiple of 8 bits.
    input += '\x80';
    //append 0 ≤ k < 512 bits '0', such that the resulting message length in bits is congruent to −64 ≡ 448 (mod 512)
    while((input.size() + 8) % 64 != 0)
        input += '\x00';
    //append ml, the original message length, as a 64-bit big-endian integer. Thus, the total length is a multiple of 512 bits.
    input += uint8_t(ml >> 56);
    input += uint8_t(ml >> 48);
    input += uint8_t(ml >> 40);
    input += uint8_t(ml >> 32);
    input += uint8_t(ml >> 24);
    input += uint8_t(ml >> 16);
    input += uint8_t(ml >> 8);
    input += uint8_t(ml);

    //Process the message in successive 512-bit chunks:
    //break message into 512-bit chunks
    for(unsigned int n=0; n<input.size(); n+=64)
    {
        //break chunk into sixteen 32-bit big-endian words w[i], 0 ≤ i ≤ 15
        uint32_t w[80];
        for(unsigned int i=0; i<16; i++)
        {
            int index = n+i*4;
            w[i] = (uint32_t(uint8_t(input[index])) << 24) | (uint32_t(uint8_t(input[index + 1])) << 16) | (uint32_t(uint8_t(input[index + 2])) << 8) | uint32_t(uint8_t(input[index + 3]));
        }
        //Extend the sixteen 32-bit words into eighty 32-bit words:
        for(unsigned int i=16; i<80; i++)
        {
            w[i] = rotate_left(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
        }

        //Initialize hash value for this chunk:
        uint32_t a = h[0];
        uint32_t b = h[1];
        uint32_t c = h[2];
        uint32_t d = h[3];
        uint32_t e = h[4];

        //Main loop:
        for(int i=0;i<80;i++)
        {
            uint32_t k, f;
            if (i < 20)
            {
                f = (b & c) ^ (~b & d);
                k = 0x5a827999;
            }
            else if (i < 40)
            {
                f = b ^ c ^ d;
                k = 0x6ed9eba1;
            }
            else if (i < 60)
            {
                f = (b & c) ^ (b & d) ^ (c & d);
                k = 0x8f1bbcdc;
            }
            else
            {
                f = b ^ c ^ d;
                k = 0xca62c1d6;
            }
            
            uint32_t temp = rotate_left(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = rotate_left(b, 30);
            b = a;
            a = temp;
        }

        //Add this chunk's hash to result so far:
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
    }

    //Produce the final hash value (big-endian) as a 160-bit number:
    result.reserve(20);
    for(int n=0; n<5; n++)
    {
        result += char(h[n] >> 24);
        result += char(h[n] >> 16);
        result += char(h[n] >> 8);
        result += char(h[n] >> 0);
    }
}

string SHA1::base64()
{
    return base64::encode(result);
}

}//namespace stringutil
}//namespace sp

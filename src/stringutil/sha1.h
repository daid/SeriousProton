#ifndef SP2_STRINGUTIL_SHA1_H
#define SP2_STRINGUTIL_SHA1_H

#include <stringImproved.h>

namespace sp {
namespace stringutil {

class SHA1
{
public:
    SHA1(string input);
    
    string base64();
private:
    string result;
};

}//namespace stringutil
}//namespace sp

#endif//SP2_STRINGUTIL_SHA1_H


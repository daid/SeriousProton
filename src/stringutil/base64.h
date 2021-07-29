#ifndef SP2_STRINGUTIL_BASE64_H
#define SP2_STRINGUTIL_BASE64_H

#include <stringImproved.h>

namespace sp {
namespace stringutil {
namespace base64 {

string encode(const string& data);
string decode(const string& data);

}//namespace base64
}//namespace stringutil
}//namespace sp

#endif//SP2_STRINGUTIL_BASE64_H


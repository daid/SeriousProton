#pragma once

#include <stdint.h>
#include <limits>


// Component to indicate that this entity exists on the server but we are running on the client.
//  And indicate which index the server has for this entity.
class ServerIndex
{
public:
    uint32_t index = std::numeric_limits<uint32_t>::max();
    uint32_t version = std::numeric_limits<uint32_t>::max();
};

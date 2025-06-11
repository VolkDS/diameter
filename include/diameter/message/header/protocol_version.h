#ifndef DIAMETER_MESSAGE_HEADER_PROTOCOL_VERSION_H
#define DIAMETER_MESSAGE_HEADER_PROTOCOL_VERSION_H

#include <cstdint>

namespace diameter::message::header {

// see RFC6733 section 3, Version
// This Version field MUST be set to 1 to indicate Diameter Version 1
using ProtocolVersion = uint8_t;

struct ProtocolVersionV
{
    enum Value : ProtocolVersion
    {
        V01 = 0x01
    };
};

}

#endif

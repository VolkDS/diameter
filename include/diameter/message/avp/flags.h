#ifndef DIAMETER_MESSAGE_AVP_FLAGS_H
#define DIAMETER_MESSAGE_AVP_FLAGS_H

#include <cstdint>
#include <type_traits>

#include <diameter/message/flags/flags.h>

namespace diameter::message::avp {

// see RFC6733
// clang-format off
enum class Flag : uint8_t
{
    VendorSpecific = 7,
    Mandatory      = 6,
    Protected      = 5,
};
// clang-format on

using Flags = diameter::message::flags::Flags<Flag, 8>;

}

#endif

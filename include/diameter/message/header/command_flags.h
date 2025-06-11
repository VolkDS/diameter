#ifndef DIAMETER_MESSAGE_HEADER_COMMAND_FLAGS_H
#define DIAMETER_MESSAGE_HEADER_COMMAND_FLAGS_H

#include <cstdint>
#include <type_traits>

#include <diameter/message/flags/flags.h>

namespace diameter::message::header {

// see RFC6733 section 3, Command Flags
enum class CommandFlag : uint8_t
{
    Request       = 7,
    Proxiable     = 6,
    Error         = 5,
    Retransmitted = 4,
};

using CommandFlags = diameter::message::flags::Flags<CommandFlag, 8>;

}

#endif

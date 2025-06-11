#ifndef DIAMETER_MESSAGE_HEADER_MESSAGE_LENGTH_H
#define DIAMETER_MESSAGE_HEADER_MESSAGE_LENGTH_H

#include <cstdint>

namespace diameter::message::header {

// see RFC6733 section 3, Message Length
using MessageLength = uint32_t;

}

#endif

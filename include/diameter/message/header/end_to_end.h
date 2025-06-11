#ifndef DIAMETER_MESSAGE_HEADER_END_TO_END_H
#define DIAMETER_MESSAGE_HEADER_END_TO_END_H

#include <cstdint>

namespace diameter::message::header {

// see RFC6733 section 3, End-to-End Identifier
using EndToEndIdentifier = uint32_t;

}

#endif

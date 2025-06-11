#ifndef DIAMETER_MESSAGE_HEADER_HOP_BY_HOP_H
#define DIAMETER_MESSAGE_HEADER_HOP_BY_HOP_H

#include <cstdint>

namespace diameter::message::header {

// see RFC6733 section 3, Hop-by-Hop Identifier
using HopByHopIdentifier = uint32_t;

}

#endif

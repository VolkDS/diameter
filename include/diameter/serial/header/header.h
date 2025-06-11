#ifndef DIAMETER_SERIAL_HEADER_HEADER_H
#define DIAMETER_SERIAL_HEADER_HEADER_H

#include <iostream>

#include <netpacker/netpacker.h>

#include <diameter/message/type_traits.h>
#include <diameter/serial/error.h>
#include <diameter/serial/header/command_flags.h>

// RFC 6733
// 3. Diameter Header
//
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |    Version    |                 Message Length                |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// | Command Flags |                  Command Code                 |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                         Application-ID                        |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                      Hop-by-Hop Identifier                    |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                      End-to-End Identifier                    |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |  AVPs ...
// +-+-+-+-+-+-+-+-+-+-+-+-+-

namespace netpacker {

template <typename OutputIt,
          typename T,
          diameter::message::is_diameter_message_header<T>* dummy = nullptr>
OutputIt put(OutputIt possition, OutputIt last, const T& value)
{
    auto pos = put(possition, last, value.version);
    pos = put(pos, last, value.length, 3);
    pos = put(pos, last, value.command_flags);
    pos = put(pos, last, value.command_code, 3);
    pos = put(pos, last, value.application_id);
    pos = put(pos, last, value.hop_by_hop);
    pos = put(pos, last, value.end_to_end);
    return pos;
}

template <typename T,
          typename InputIt,
          diameter::message::is_diameter_message_header<T>* dummy = nullptr>
T get(InputIt& possition, InputIt last)
{
    T value;

    value.version = get<decltype(value.version)>(possition, last);
    if (value.version != diameter::message::header::ProtocolVersionV::V01) {
        throw diameter::serial::InvalidProtocolVersion();
    }

    value.length = get<decltype(value.length)>(possition, last, 3);
    if (value.length < value.size()) {
        throw diameter::serial::InvalidMessageLength();
    }

    value.command_flags = get<decltype(value.command_flags)>(possition, last);
    value.command_code = get<decltype(value.command_code)>(possition, last, 3);
    value.application_id = get<decltype(value.application_id)>(possition, last);
    value.hop_by_hop = get<decltype(value.hop_by_hop)>(possition, last);
    value.end_to_end = get<decltype(value.end_to_end)>(possition, last);
    return value;
}

}

#endif

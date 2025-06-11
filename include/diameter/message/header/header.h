#ifndef DIAMETER_MESSAGE_HEADER_HEADER_H
#define DIAMETER_MESSAGE_HEADER_HEADER_H

#include <diameter/message/header/application_id.h>
#include <diameter/message/header/command_code.h>
#include <diameter/message/header/command_flags.h>
#include <diameter/message/header/end_to_end.h>
#include <diameter/message/header/hop_by_hop.h>
#include <diameter/message/header/message_length.h>
#include <diameter/message/header/protocol_version.h>

namespace diameter::message::header {

struct Header
{
    ProtocolVersion    version;
    MessageLength      length;
    CommandFlags       command_flags;
    CommandCode        command_code;
    ApplicationId      application_id;
    HopByHopIdentifier hop_by_hop;
    EndToEndIdentifier end_to_end;

    Header() = default;

    Header(Header const&) = default;
    Header& operator= (Header const&) = default;
    Header(Header&&) = default;
    Header& operator= (Header&&) = default;

    constexpr MessageLength size() const
    {
        return sizeof(ProtocolVersion)
            + sizeof(MessageLength) - 1
            + sizeof(CommandFlags::UnderlyingT)
            + sizeof(CommandCode) - 1
            + sizeof(ApplicationId)
            + sizeof(HopByHopIdentifier)
            + sizeof(EndToEndIdentifier);
    }
};

}

#endif

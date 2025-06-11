#ifndef DIAMETER_MESSAGE_MESSAGE_H
#define DIAMETER_MESSAGE_MESSAGE_H

#include <list>

#include <diameter/message/avp/avp.h>
#include <diameter/message/header/header.h>

namespace diameter::message {

struct Message
{
    header::Header header;
    std::list<avp::AVP> avps;

    Message() = default;

    Message(Message const&) = default;
    Message& operator= (Message const&) = default;
    Message(Message&&) = default;
    Message& operator= (Message&&) = default;

    header::MessageLength size()
    {
        header.length = header.size();
        for (const auto& avp : avps) {
            header.length += avp.size();
        }
        return header.length;
    }
};

} // namespace diameter::message

#endif

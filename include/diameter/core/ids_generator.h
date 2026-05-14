#ifndef DIAMETER_CORE_CONTROLLER_IDS_GENERATOR_H
#define DIAMETER_CORE_CONTROLLER_IDS_GENERATOR_H

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <ctime>

#include <diameter/message/header/end_to_end.h>
#include <diameter/message/header/hop_by_hop.h>

namespace diameter::core {

// Hop-by-Hop Identifier
// The Hop-by-Hop Identifier is an unsigned 32-bit integer field (in network byte order) that aids
// in matching requests and replies. The sender MUST ensure that the Hop-by-Hop Identifier in a
// request is unique on a given connection at any given time, and it MAY attempt to ensure that the
// number is unique across reboots. The sender of an answer message MUST ensure that the
// Hop-by-Hop Identifier field contains the same value that was found in the corresponding request.
// The Hop-by-Hop Identifier is normally a monotonically increasing number, whose start value was
// randomly generated. An answer message that is received with an unknown Hop-by-Hop Identifier
// MUST be discarded.

// End-to-End Identifier
// The End-to-End Identifier is an unsigned 32-bit integer field (in network byte order) that is
// used to detect duplicate messages. Upon reboot, implementations MAY set the high order 12 bits
// to contain the low order 12 bits of current time, and the low order 20 bits to a random value.
// Senders of request messages MUST insert a unique identifier on each message.  The identifier
// MUST remain locally unique for a period of at least 4 minutes, even across reboots.
// The originator of an answer message MUST ensure that the End-to-End Identifier field contains
// the same value that was found in the corresponding request. The End-to-End Identifier MUST NOT
// be modified by Diameter agents of any kind. The combination of the Origin-Host AVP
// (Section 6.3) and this field is used to detect duplicates.  Duplicate requests SHOULD cause the
// same answer to be transmitted (modulo the Hop-by-Hop Identifier field and any routing AVPs that
// may be present), and they MUST NOT affect any state that was set when the original request was
// processed. Duplicate answer messages that are to be locally consumed (see Section 6.2) SHOULD
// be silently discarded.

class IdsGenerator
{
public:
    IdsGenerator()
        : m_hop_by_hop(static_cast<message::header::HopByHopIdentifier>(std::rand())),
          m_end_to_end(
              // the high order 12 bits to contain the low order 12 bits of current time
              ((static_cast<std::uint32_t>(std::time(nullptr)) & 0xFFF) << 20) |
              // the low order 20 bits to a random value
              (static_cast<std::uint32_t>(std::rand()) & 0xFFFFF))
    {
    }

    IdsGenerator(IdsGenerator const&) = delete;
    IdsGenerator& operator= (IdsGenerator const&) = delete;
    IdsGenerator(IdsGenerator&&) = delete;
    IdsGenerator& operator= (IdsGenerator&&) = delete;

    message::header::HopByHopIdentifier next_hop_by_hop()
    {
        return m_hop_by_hop.fetch_add(1, std::memory_order_relaxed);
    }

    message::header::EndToEndIdentifier next_end_to_end()
    {
        return m_end_to_end.fetch_add(1, std::memory_order_relaxed);
    }

private:
    std::atomic<message::header::HopByHopIdentifier> m_hop_by_hop;
    std::atomic<message::header::EndToEndIdentifier> m_end_to_end;
};

}

#endif

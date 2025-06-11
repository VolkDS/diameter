#ifndef DIAMETER_SERIAL_MESSAGE_H
#define DIAMETER_SERIAL_MESSAGE_H

#include <netpacker/netpacker.h>

#include <diameter/message/type_traits.h>

namespace netpacker {

template <typename OutputIt,
          typename T,
          diameter::message::is_diameter_message<T>* dummy = nullptr>
OutputIt put(OutputIt possition, OutputIt last, const T& value)
{
    auto pos = put(possition, last, value.header);
    for (const auto& avp : value.avps) {
        pos = put(pos, last, avp);
    }
    return pos;
}

template <typename T,
          typename InputIt,
          diameter::message::is_diameter_message<T>* dummy = nullptr>
T get(InputIt& possition, InputIt last)
{
    T value;
    value.header = get<decltype(value.header)>(possition, last);

    auto alen = value.header.length;
    // TODO: What to do if length low than possition-last ?
    while (alen > 20) {
        auto avp = get<diameter::message::avp::AVP>(possition, last);
        alen -= avp.size();
        value.avps.emplace_back(avp);
    }

    return value;
}

}

#endif

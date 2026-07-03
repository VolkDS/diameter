#ifndef DIAMETER_SERIAL_AVP_FLAGS_H
#define DIAMETER_SERIAL_AVP_FLAGS_H

#include <netpacker/netpacker.h>

#include <diameter/message/type_traits.h>

namespace netpacker {

// clang-format off
template <typename OutputIt,
          typename T,
          diameter::message::is_diameter_message_avp_flags<T>* dummy = nullptr>
// clang-format on
OutputIt put(OutputIt possition, OutputIt last, const T& value)
{
    return put(possition, last, value.data(), value.size());
}

// clang-format off
template <typename T,
          typename InputIt,
          diameter::message::is_diameter_message_avp_flags<T>* dummy = nullptr>
// clang-format on
T get(InputIt& possition, InputIt last)
{
    auto flags = get<typename T::UnderlyingT>(possition, last);
    return T(flags);
}

}

#endif

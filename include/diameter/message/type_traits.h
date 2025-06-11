#ifndef DIAMETER_MESSAGE_TYPE_TRAITS_H
#define DIAMETER_MESSAGE_TYPE_TRAITS_H

#include <diameter/message/avp/avp.h>
#include <diameter/message/header/header.h>
#include <diameter/message/message.h>

namespace diameter::message {

template<typename T>
using is_diameter_message = 
    typename std::enable_if_t<std::is_same_v<T, Message>, bool>;

template<typename T>
using is_diameter_message_avp =
    typename std::enable_if_t<std::is_same_v<T, avp::AVP>, bool>;

template<typename T>
using is_diameter_message_avp_flags =
    typename std::enable_if_t<std::is_same_v<T, avp::Flags>, bool>;

template<typename T>
using is_diameter_message_avp_value =
    typename std::enable_if_t<std::is_same_v<T, avp::Value>, bool>;

template<typename T>
using is_diameter_message_avp_address =
    typename std::enable_if_t<std::is_same_v<T, avp::Address>, bool>;

template<typename T>
using is_diameter_message_header =
    typename std::enable_if_t<std::is_same_v<T, header::Header>, bool>;

template<typename T>
using is_diameter_message_header_command_flags =
    typename std::enable_if_t<std::is_same_v<T, header::CommandFlags>, bool>;

}

#endif /* DIAMETER_MESSAGE_TYPE_TRAITS_H */

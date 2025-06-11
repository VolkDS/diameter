#ifndef DIAMETER_SERIAL_AVP_AVP_H
#define DIAMETER_SERIAL_AVP_AVP_H

#include <exception>

#include <netpacker/netpacker.h>

#include <diameter/message/avp/avp.h>
#include <diameter/message/type_traits.h>
#include <diameter/serial/avp/flags.h>
#include <diameter/serial/error.h>

// RFC 6733
// 4.1.  AVP Header
//
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                           AVP Code                            |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |V M P r r r r r|                  AVP Length                   |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                        Vendor-ID (opt)                        |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |    Data ...
// +-+-+-+-+-+-+-+-+

namespace diameter::serial::avp {

template<typename To>
To value_as(const message::avp::Value& value);

}

namespace netpacker {

template <typename T,
          typename InputIt,
          diameter::message::is_diameter_message_avp_value<T>* dummy = nullptr>
T get(InputIt& possition, InputIt last, typename std::iterator_traits<InputIt>::difference_type len)
{
    namespace dma = diameter::message::avp;
    auto raw_data = get<dma::OctetString::value_type>(possition, last, len);
    auto value = dma::OctetString(std::move(raw_data));
    return T {std::move(value)};
}

template <typename OutputIt,
            typename T,
            diameter::message::is_diameter_message_avp_value<T>* dummy = nullptr>
OutputIt put(OutputIt possition, OutputIt last, const T& value)
{
    namespace dma = diameter::message::avp;
    namespace dsa = diameter::serial::avp;
    const auto& octet_string_avp = dsa::value_as<dma::OctetString>(value);
    return put(possition, last, *octet_string_avp);
}

template <typename T,
          typename InputIt,
          diameter::message::is_diameter_message_avp_address<T>* dummy = nullptr>
T get(InputIt& possition, InputIt last)
{
    namespace dma = diameter::message::avp;

    auto address_family = get<typename T::value_type::address_family_type>(possition, last);
    auto address_value
        = get<typename T::value_type::value_type>(possition, last, std::distance(possition, last));

    auto value = dma::Address(address_family, address_value);
    return T {std::move(value)};
}


template <typename OutputIt,
            typename T,
            diameter::message::is_diameter_message_avp<T>* dummy = nullptr>
OutputIt put(OutputIt possition, OutputIt last, const T& value)
{
    auto pos = put(possition, last, value.code);
    pos = put(pos, last, value.flags);
    pos = put(pos, last, value.length(), 3);

    if (value.flags[diameter::message::avp::Flag::VendorSpecific]) {
        if (!value.vendor_id.has_value()) {
            throw diameter::serial::InvalidAvpVendorId();
        }
        pos = put(pos, last, value.vendor_id.value());
    }

    pos = put(pos, last, value.value);
    pos = skipbytes(pos, last, value.padding());
    return pos;
}

template <typename T,
            typename InputIt,
            diameter::message::is_diameter_message_avp<T>* dummy = nullptr>
T get(InputIt& possition, InputIt last)
{
    T value;
    value.code = get<decltype(value.code)>(possition, last);
    value.flags = get<decltype(value.flags)>(possition, last);

    auto length = get<diameter::message::avp::Length>(possition, last, 3);
    if (length < 8) {
        throw diameter::serial::InvalidAvpLength("Invalid avp length: too small");
    }
    length -= 8;

    if (value.flags[diameter::message::avp::Flag::VendorSpecific]) {
        if (length < 4) {
            throw diameter::serial::InvalidAvpLength(
                "Invalid avp length: too small for vendor specific");
        }
        value.vendor_id = get<diameter::message::avp::VendorId>(possition, last);
        length -= 4;
    }

    value.value = get<diameter::message::avp::Value>(possition, last, length);
    possition = skipbytes(possition, last, value.padding());
    return value;
}

} // namespace netpacker

namespace diameter::serial::avp {

template<typename From, typename To>
To cast(const From& value)
{
    using RawValue = typename From::value_type;
    auto& from_raw_value = *value;
    if constexpr (std::is_same_v<message::avp::BasicValue<RawValue>, std::decay_t<To>>) {
        return from_raw_value;
    }
    else if constexpr (std::is_same_v<message::avp::BasicValue<RawValue>,
                           std::decay_t<message::avp::OctetString>>) {
        if constexpr (std::is_arithmetic_v<typename To::value_type>) {
            // OctetString -> Integer32 / Integer64 / Unsigned32 / Unsigned64 / Float32 / Float64
            if (from_raw_value.size() != sizeof(typename To::value_type)) {
                throw diameter::serial::InvalidAvpValueCast(
                    "BasicValue::cast: invalid size of OctetString for cast");
            }
            auto pos = from_raw_value.begin();
            auto raw_value = netpacker::get<typename To::value_type>(pos, from_raw_value.end());
            return To(raw_value);
        }
        else if constexpr (std::is_same_v<std::decay_t<To>, std::decay_t<message::avp::Time>>) {
            // OctetString -> Time
            if (from_raw_value.size() != sizeof(typename To::value_type::value_type)) {
                throw diameter::serial::InvalidAvpValueCast(
                    "BasicValue::cast: invalid size of OctetString for cast");
            }
            auto pos = from_raw_value.begin();
            auto raw_value
                = netpacker::get<typename To::value_type::value_type>(pos, from_raw_value.end());
            return To(raw_value);
        }
        else if constexpr (std::is_same_v<std::decay_t<To>, std::decay_t<message::avp::Address>>) {
            // OctetString -> Address
            auto pos = from_raw_value.begin();
            auto raw_value = netpacker::get<message::avp::Address>(pos, from_raw_value.end());
            return To(std::move(raw_value));
        }
        else if constexpr (
            std::is_same_v<std::decay_t<To>, std::decay_t<message::avp::UTF8String>>) {
            // OctetString -> UTF8String
            auto pos = from_raw_value.begin();
            auto raw_value = netpacker::get<typename To::value_type>(pos, from_raw_value.end(),
                std::distance(pos, from_raw_value.end()));
            return To(raw_value);
        }
        else if constexpr (
            std::is_same_v<std::decay_t<To>, std::decay_t<message::avp::DiameterIdentity>>) {
            // OctetString -> DiameterIdentity
            auto pos = from_raw_value.begin();
            auto raw_value = netpacker::get<typename To::value_type::value_type>(pos,
                from_raw_value.end(), std::distance(pos, from_raw_value.end()));
            return To(raw_value);
        }
        else if constexpr (
            std::is_same_v<std::decay_t<To>, std::decay_t<message::avp::DiameterURI>>) {
            // OctetString -> DiameterURI
            auto pos = from_raw_value.begin();
            auto raw_value = netpacker::get<typename To::value_type::value_type>(pos,
                from_raw_value.end(), std::distance(pos, from_raw_value.end()));
            return To(raw_value);
        }
        else if constexpr (
            std::is_same_v<std::decay_t<To>, std::decay_t<message::avp::Enumerated>>) {
            // OctetString -> Enumerated
            if (from_raw_value.size() != sizeof(typename To::value_type::value_type)) {
                throw diameter::serial::InvalidAvpValueCast(
                    "BasicValue::cast: invalid size of OctetString for cast");
            }
            auto pos = from_raw_value.begin();
            auto raw_value
                = netpacker::get<typename To::value_type::value_type>(pos, from_raw_value.end());
            return To(raw_value);
        }
        else if constexpr (
            std::is_same_v<std::decay_t<To>, std::decay_t<message::avp::IPFilterRule>>) {
            // OctetString -> IPFilterRule
            auto pos = from_raw_value.begin();
            auto raw_value = netpacker::get<typename To::value_type::value_type>(pos,
                from_raw_value.end(), std::distance(pos, from_raw_value.end()));
            return To(raw_value);
        }
        else if constexpr (std::is_same_v<std::decay_t<To>, std::decay_t<message::avp::Grouped>>) {
            // OctetString -> Grouped
            auto pos = from_raw_value.begin();
            auto alen = from_raw_value.size();
            typename To::value_type raw_value;
            while (alen > 0) {
                auto avp = netpacker::get<typename To::value_type::value_type>(pos,
                    from_raw_value.end());
                alen -= avp.size();
                raw_value.emplace_back(avp);
            }
            return To(raw_value);
        }
    }
    else if constexpr (std::is_arithmetic_v<RawValue>) {
        if constexpr (std::is_same_v<To, std::decay_t<message::avp::OctetString>>) {
            // Integer32 / Integer64 / Unsigned32 / Unsigned64 / Float32 / Float64 -> OctetString
            typename To::value_type octet_string_v(value.size());
            netpacker::put(octet_string_v.begin(), octet_string_v.end(), from_raw_value);
            return To(octet_string_v);
        }
    }
    else if constexpr (
        std::is_same_v<message::avp::BasicValue<RawValue>, std::decay_t<message::avp::Time>>) {
        if constexpr (std::is_same_v<To, std::decay_t<message::avp::OctetString>>) {
            // Time -> OctetString
            typename To::value_type octet_string_v(value.size());
            netpacker::put(octet_string_v.begin(), octet_string_v.end(), from_raw_value.value());
            return To(octet_string_v);
        }
    }
    else if constexpr (
        std::is_same_v<message::avp::BasicValue<RawValue>, std::decay_t<message::avp::Address>>) {
        if constexpr (std::is_same_v<To, std::decay_t<message::avp::OctetString>>) {
            // Address -> OctetString
            typename To::value_type octet_string_v(value.size());
            auto pos = octet_string_v.begin();
            pos = netpacker::put(pos, octet_string_v.end(), from_raw_value.address_family());
            pos = netpacker::put(pos, octet_string_v.end(), from_raw_value.value());
            return To(octet_string_v);
        }
    }
    else if constexpr (std::is_same_v<message::avp::BasicValue<RawValue>,
                           std::decay_t<message::avp::UTF8String>>) {
        if constexpr (std::is_same_v<To, std::decay_t<message::avp::OctetString>>) {
            // UTF8String -> OctetString
            typename To::value_type octet_string_v(value.size());
            netpacker::put(octet_string_v.begin(), octet_string_v.end(), from_raw_value);
            return To(octet_string_v);
        }
    }
    else if constexpr (std::is_same_v<message::avp::BasicValue<RawValue>,
                           std::decay_t<message::avp::DiameterIdentity>>) {
        if constexpr (std::is_same_v<To, std::decay_t<message::avp::OctetString>>) {
            // DiameterIdentity -> OctetString
            typename To::value_type octet_string_v(value.size());
            netpacker::put(octet_string_v.begin(), octet_string_v.end(), from_raw_value.value());
            return To(octet_string_v);
        }
    }
    else if constexpr (std::is_same_v<message::avp::BasicValue<RawValue>,
                           std::decay_t<message::avp::DiameterURI>>) {
        if constexpr (std::is_same_v<To, std::decay_t<message::avp::OctetString>>) {
            // DiameterURI -> OctetString
            typename To::value_type octet_string_v(value.size());
            netpacker::put(octet_string_v.begin(), octet_string_v.end(), from_raw_value.value());
            return To(octet_string_v);
        }
    }
    else if constexpr (std::is_same_v<message::avp::BasicValue<RawValue>,
                           std::decay_t<message::avp::Enumerated>>) {
        if constexpr (std::is_same_v<To, std::decay_t<message::avp::OctetString>>) {
            // Enumerated -> OctetString
            typename To::value_type octet_string_v(value.size());
            netpacker::put(octet_string_v.begin(), octet_string_v.end(), from_raw_value.value());
            return To(octet_string_v);
        }
    }
    else if constexpr (std::is_same_v<message::avp::BasicValue<RawValue>,
                           std::decay_t<message::avp::IPFilterRule>>) {
        if constexpr (std::is_same_v<To, std::decay_t<message::avp::OctetString>>) {
            // IPFilterRule -> OctetString
            typename To::value_type octet_string_v(value.size());
            netpacker::put(octet_string_v.begin(), octet_string_v.end(), from_raw_value.value());
            return To(octet_string_v);
        }
    }
    else if constexpr (
        std::is_same_v<message::avp::BasicValue<RawValue>, std::decay_t<message::avp::Grouped>>) {
        if constexpr (std::is_same_v<To, std::decay_t<message::avp::OctetString>>) {
            // Grouped -> OctetString
            typename To::value_type octet_string_v(value.size());
            auto pos = octet_string_v.begin();
            for (const auto& v : from_raw_value) {
                pos = netpacker::put(pos, octet_string_v.end(), v);
            }
            return To(octet_string_v);
        }
    }
    throw diameter::serial::InvalidAvpValueCast("BasicValue::cast: unsupported cast");
}

template<typename To>
To value_as(const message::avp::Value& value)
{
    return std::visit(
        [](auto&& arg) -> To {
            using From = typename std::decay<decltype(arg)>::type;
            return cast<From, To>(arg);
        },
        value);
}

} // namespace diameter::serial::avp

#endif

#ifndef DIAMETER_MESSAGE_AVP_VALUE_ADDRESS_H
#define DIAMETER_MESSAGE_AVP_VALUE_ADDRESS_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <arpa/inet.h>

namespace diameter::message::avp::value {

struct AddressFamilyV
{
    // https://www.iana.org/assignments/address-family-numbers/address-family-numbers.xhtml
    enum Value : uint16_t
    {
        IPv4 = 1,
        IPv6 = 2,
        Nsap = 3,
        Hdlc = 4,
        BBN1822 = 5,
        IEEE802 = 6,
        E163 = 7,
        E164 = 8,
        F69 = 9,
        FrameRelay = 10,
        IPX = 11,
        AppleTalk = 12,
        DecnetIv = 13,
        BanyanVines = 14,
        E164Nsap = 15,
        DNS = 16,
        // and other...
    };
};

/*
 * The Address format is derived from the OctetString Basic AVP Format.  It is a discriminated union
 * representing, for example, a 32-bit (IPv4) [RFC0791] or 128-bit (IPv6) [RFC4291] address, most
 * significant octet first.  The first two octets of the Address AVP represent the AddressType,
 * which contains an Address Family, defined in [IANAADFAM].  The AddressType is used to
 * discriminate the content and format of the remaining octets.
 */
class AddressBase
{
public:
    using value_type = std::vector<uint8_t>;
    using address_family_type = uint16_t;
    using address_string_type = std::string;

    AddressBase() = default;

    AddressBase(AddressBase const&) = default;
    AddressBase& operator= (AddressBase const&) = default;
    AddressBase(AddressBase&&) = default;
    AddressBase& operator= (AddressBase&&) = default;

    AddressBase(address_family_type address_family, const value_type& val)
        : m_address_family(address_family),
          m_value(val)
    {
    }

    AddressBase(address_family_type address_family, const address_string_type& val)
        : m_address_family(address_family),
          m_address_string(val)
    {
        if (m_address_family == AddressFamilyV::IPv4) {
            struct in_addr ipv4_bin;
            if (inet_pton(AF_INET, m_address_string.c_str(), &ipv4_bin) < 1) {
                throw std::runtime_error("avp::Address: incorrect IPv4");
            }
            m_value.resize(sizeof(in_addr));
            std::memcpy(m_value.data(), &ipv4_bin, sizeof(ipv4_bin));
        }
        else if (m_address_family == AddressFamilyV::IPv6) {
            struct in6_addr ipv6_bin;
            if (inet_pton(AF_INET6, m_address_string.c_str(), &ipv6_bin) < 1) {
                throw std::runtime_error("avp::Address: incorrect IPv6");
            }
            m_value.resize(sizeof(in6_addr));
            std::memcpy(m_value.data(), &ipv6_bin, sizeof(in6_addr));
        }
        else {
            m_value.assign(m_address_string.begin(), m_address_string.end());
        }
    }

    AddressBase(const address_string_type& val)
        : AddressBase(AddressFamilyV::IPv4, val)
    {
    }

    value_type value() const
    {
        return m_value;
    }

    address_family_type address_family() const
    {
        return m_address_family;
    }

    address_string_type address_string() const
    {
        return m_address_string;
    }

    std::size_t size() const noexcept
    {
        return m_value.size() + sizeof(address_family_type);
    }

    bool is_ipv4() const noexcept
    {
        return (m_address_family == AddressFamilyV::IPv4) && (m_value.size() == sizeof(in_addr));
    }

    bool is_ipv6() const noexcept
    {
        return (m_address_family == AddressFamilyV::IPv6) && (m_value.size() == sizeof(in6_addr));
    }

    bool validate()
    {
        if (m_address_family == AddressFamilyV::IPv4) {
            if (m_value.size() != sizeof(in_addr)) {
                return m_is_validated;
            }

            struct in_addr ipv4_bin;
            std::memcpy(&ipv4_bin, m_value.data(), sizeof(ipv4_bin));

            char ipv4_str[INET_ADDRSTRLEN];
            const char* result = inet_ntop(AF_INET, &ipv4_bin, ipv4_str, INET_ADDRSTRLEN);
            if (result == nullptr) {
                return m_is_validated;
            }
            m_address_string.assign(result);
            m_is_validated = true;
            return m_is_validated;
        }
        else if (m_address_family == AddressFamilyV::IPv6) {
            if (m_value.size() != sizeof(in6_addr)) {
                return m_is_validated;
            }

            struct in6_addr ipv6_bin;
            std::memcpy(&ipv6_bin, m_value.data(), sizeof(ipv6_bin));

            char ipv6_str[INET6_ADDRSTRLEN];
            const char* result = inet_ntop(AF_INET6, &ipv6_bin, ipv6_str, INET6_ADDRSTRLEN);
            if (result == nullptr) {
                return m_is_validated;
            }
            m_address_string.assign(result);
            m_is_validated = true;
            return m_is_validated;
        }
        return m_is_validated;
    }

private:
    address_family_type m_address_family;
    value_type m_value;

    bool m_is_validated {false};
    address_string_type m_address_string;
};

} // namespace diameter::message::avp::value

#endif

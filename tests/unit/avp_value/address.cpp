#include <boost/test/unit_test.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <diameter/message/avp/avp.h>

using namespace diameter;

BOOST_AUTO_TEST_SUITE(avp_address)

BOOST_AUTO_TEST_CASE(constructor)
{
    // Test case 1: IPv4 default constructor
    auto value1 = message::avp::Address("127.0.0.1");
    BOOST_CHECK_EQUAL(value1->address_string(), "127.0.0.1");
    BOOST_CHECK_EQUAL(value1->address_family(), message::avp::value::AddressFamilyV::IPv4);

    // Test case 2: Explicit IPv4 constructor
    auto value2 = message::avp::Address(message::avp::value::AddressFamilyV::IPv4, "192.168.1.1");
    BOOST_CHECK_EQUAL(value2->address_string(), "192.168.1.1");
    BOOST_CHECK_EQUAL(value2->address_family(), message::avp::value::AddressFamilyV::IPv4);

    // Test case 3: IPv6 address
    auto value3 = message::avp::Address(message::avp::value::AddressFamilyV::IPv6,
        "2001:db8:3c4d:7777:260:3eff:fe15:9501");
    BOOST_CHECK_EQUAL(value3->address_string(), "2001:db8:3c4d:7777:260:3eff:fe15:9501");
    BOOST_CHECK_EQUAL(value3->address_family(), message::avp::value::AddressFamilyV::IPv6);

    // Test case 4: Invalid IPv4 address (out of range)
    BOOST_CHECK_THROW(message::avp::Address("127.0.0.256"), std::runtime_error);

    // Test case 5: Invalid IPv4 format
    BOOST_CHECK_THROW(message::avp::Address("127.0.0"), std::runtime_error);

    // Test case 6: IPv4 with leading zeros (should be valid but normalized)
    // auto value6 = message::avp::Address("010.020.030.040");
    // BOOST_CHECK_EQUAL(value6->address_string(), "10.20.30.40");

    auto value7 = message::avp::Address(message::avp::value::AddressFamilyV::IPv6, "::1");
    BOOST_CHECK_EQUAL(value7->address_string(), "::1");

    // Test case 8: IPv4-mapped IPv6 address
    auto value8
        = message::avp::Address(message::avp::value::AddressFamilyV::IPv6, "::ffff:192.168.1.1");
    BOOST_CHECK_EQUAL(value8->address_string(), "::ffff:192.168.1.1");

    // Test case 9: Empty string (invalid)
    BOOST_CHECK_THROW(message::avp::Address(""), std::runtime_error);

    // Test case 10: IPv6 with mixed case
    //auto value10 = message::avp::Address(message::avp::value::AddressFamilyV::IPv6, "2001:dB8:3C4d:7777:260:3eFf:Fe15:9501");
    //BOOST_CHECK_EQUAL(value10->address_string(), "2001:db8:3c4d:7777:260:3eff:fe15:9501");

    // Test case 11: Minimum valid IPv4
    auto value11 = message::avp::Address("0.0.0.0");
    BOOST_CHECK_EQUAL(value11->address_string(), "0.0.0.0");

    // Test case 12: Maximum valid IPv4
    auto value12 = message::avp::Address("255.255.255.255");
    BOOST_CHECK_EQUAL(value12->address_string(), "255.255.255.255");

    // Test case 13: Invalid IPv6 (too many segments)
    BOOST_CHECK_THROW(message::avp::Address(message::avp::value::AddressFamilyV::IPv6,
                          "2001:DB8:3C4D:7777:260:3EFF:FE15:9501:1234"),
        std::runtime_error);

    // Test case 14: IPv4 with wrong family specified
    BOOST_CHECK_THROW(message::avp::Address(message::avp::value::AddressFamilyV::IPv6,
                          "192.168.1.1"),
        std::runtime_error);

    // Test case 15: IPv6 with wrong family specified
    BOOST_CHECK_THROW(message::avp::Address(message::avp::value::AddressFamilyV::IPv4, "::1"),
        std::runtime_error);
}

BOOST_AUTO_TEST_CASE(ipv4_payload_is_network_order_and_excludes_family)
{
    const auto address = message::avp::Address("192.0.2.129");
    const std::vector<uint8_t> expected{192, 0, 2, 129};
    const auto bytes = address->value();
    BOOST_CHECK_EQUAL_COLLECTIONS(bytes.begin(), bytes.end(), expected.begin(), expected.end());
    BOOST_CHECK_EQUAL(address.size(), 6);
    BOOST_CHECK(address->is_ipv4());
    BOOST_CHECK(!address->is_ipv6());
}

BOOST_AUTO_TEST_CASE(ipv6_payload_is_network_order_and_includes_family_in_size)
{
    const auto address = message::avp::Address(message::avp::value::AddressFamilyV::IPv6,
        "2001:db8::1");
    const std::vector<uint8_t> expected{
        0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    const auto bytes = address->value();
    BOOST_CHECK_EQUAL_COLLECTIONS(bytes.begin(), bytes.end(), expected.begin(), expected.end());
    BOOST_CHECK_EQUAL(address.size(), 18);
    BOOST_CHECK(address->is_ipv6());
    BOOST_CHECK(!address->is_ipv4());
}

BOOST_AUTO_TEST_CASE(binary_ipv4_validation_builds_presentation_string)
{
    auto address = message::avp::Address(message::avp::value::AddressFamilyV::IPv4,
        std::vector<uint8_t>{192, 0, 2, 1});
    BOOST_REQUIRE(address->validate());
    BOOST_CHECK_EQUAL(address->address_string(), "192.0.2.1");
    BOOST_CHECK(address->validate());
    BOOST_CHECK_EQUAL(address.size(), 6);
}

BOOST_AUTO_TEST_CASE(binary_ipv6_validation_builds_presentation_string)
{
    auto address = message::avp::Address(message::avp::value::AddressFamilyV::IPv6,
        std::vector<uint8_t>{0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1});
    BOOST_REQUIRE(address->validate());
    BOOST_CHECK_EQUAL(address->address_string(), "2001:db8::1");
    BOOST_CHECK(address->validate());
    BOOST_CHECK_EQUAL(address.size(), 18);
}

BOOST_AUTO_TEST_CASE(binary_ip_payload_lengths_must_match_family)
{
    for (std::size_t size = 0; size <= 17; ++size) {
        BOOST_TEST_CONTEXT("payload size " << size) {
            auto ipv4 = message::avp::Address(message::avp::value::AddressFamilyV::IPv4,
                std::vector<uint8_t>(size));
            auto ipv6 = message::avp::Address(message::avp::value::AddressFamilyV::IPv6,
                std::vector<uint8_t>(size));
            BOOST_CHECK_EQUAL(ipv4->validate(), size == 4);
            BOOST_CHECK_EQUAL(ipv4->is_ipv4(), size == 4);
            BOOST_CHECK_EQUAL(ipv6->validate(), size == 16);
            BOOST_CHECK_EQUAL(ipv6->is_ipv6(), size == 16);
        }
    }
}

BOOST_AUTO_TEST_CASE(unsupported_binary_family_preserves_opaque_bytes)
{
    const std::vector<uint8_t> expected{0, 1, 0xff};
    auto address = message::avp::Address(uint16_t{65535}, expected);
    BOOST_CHECK(!address->validate());
    BOOST_CHECK(!address->is_ipv4());
    BOOST_CHECK(!address->is_ipv6());
    BOOST_CHECK_EQUAL(address->address_family(), 65535);
    BOOST_CHECK_EQUAL(address.size(), 5);
    const auto actual = address->value();
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(text_address_rejects_embedded_nul_suffix)
{
    const std::string ipv4{"127.0.0.1\0invalid", 17};
    const std::string ipv6{"::1\0invalid", 11};
    BOOST_CHECK_THROW(static_cast<void>(message::avp::Address{ipv4}), std::runtime_error);
    BOOST_CHECK_THROW(message::avp::Address(message::avp::value::AddressFamilyV::IPv6, ipv6),
        std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()

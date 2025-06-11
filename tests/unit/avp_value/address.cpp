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
    auto value3 = message::avp::Address(message::avp::value::AddressFamilyV::IPv6, "2001:db8:3c4d:7777:260:3eff:fe15:9501");
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
    auto value8 = message::avp::Address(message::avp::value::AddressFamilyV::IPv6, "::ffff:192.168.1.1");
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
        "2001:DB8:3C4D:7777:260:3EFF:FE15:9501:1234"), std::runtime_error);

    // Test case 14: IPv4 with wrong family specified
    BOOST_CHECK_THROW(message::avp::Address(message::avp::value::AddressFamilyV::IPv6, "192.168.1.1"),
        std::runtime_error);

    // Test case 15: IPv6 with wrong family specified
    BOOST_CHECK_THROW(message::avp::Address(message::avp::value::AddressFamilyV::IPv4, "::1"),
        std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()

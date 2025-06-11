#include <boost/test/unit_test.hpp>

#include <iostream>
#include <iomanip>
#include <vector>

#include <netpacker/error.h>

#include <diameter/serial/error.h>
#include <diameter/serial/avp/avp.h>

#include "../../helpers/string_to_time.h"

using namespace diameter::message::avp;

BOOST_AUTO_TEST_SUITE(avp_value)

BOOST_AUTO_TEST_CASE(decode)
{
    auto data = std::vector<uint8_t>{
        0x00, 0x00, 0x00, 0x01,   //Code
        0b10000000,               //Flags
              0x00, 0x00, 0x10,   //Length
        0x00, 0x00, 0x00, 0x01,   //VendorID
        0x00, 0x00, 0x00, 0x01    //Data
    };

    auto value0 = OctetString();
    auto value1 = OctetString(data.begin(), data.end());
    //auto value1 = OctetString{0x00, 0x00, 0x00, 0x01};
    auto value2 = OctetString(data);
    //auto value3 = Unsigned64(1);

    //auto value4 = OctetString(value0);
    //auto value5 = OctetString(value3);

    BOOST_CHECK_EQUAL(value1->size(), data.size());
    BOOST_CHECK_EQUAL(value2->size(), data.size());
    //BOOST_CHECK_EQUAL(sizeof(value3.get()), 8);
}

BOOST_AUTO_TEST_CASE(time)
{
    /*
        AVP: Last-UE-Activity-Time(1494) l=16 f=V-- vnd=TGPP val=May 21, 2024 08:01:09.000000000 UTC
            AVP Code: 1494 Last-UE-Activity-Time
            AVP Flags: 0x80, Vendor-Specific: Set
            AVP Length: 16
            AVP Vendor Id: 3GPP (10415)
            Last-UE-Activity-Time: May 21, 2024 08:01:09.000000000 UTC
    */
    auto data = std::vector<uint8_t>{
        0x00, 0x00, 0x05, 0xd6,
        0x80,
              0x00, 0x00, 0x10,
        0x00, 0x00, 0x28, 0xaf,
        0xe9, 0xf6, 0xd3, 0x45
    };

    auto pos = data.begin();
    auto avp = netpacker::get<AVP>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(avp.code, 1494);
    BOOST_CHECK_EQUAL(avp.flags[Flag::VendorSpecific], true);
    BOOST_CHECK_EQUAL(avp.flags[Flag::Mandatory], false);
    BOOST_CHECK_EQUAL(avp.flags[Flag::Protected], false);
    BOOST_CHECK_EQUAL(avp.length(), 16);
    BOOST_CHECK_EQUAL(avp.size(), 16);
    BOOST_CHECK_EQUAL(avp.vendor_id.has_value(), true);
    BOOST_CHECK_EQUAL(avp.vendor_id.value(), 10415);

    auto value1 = diameter::serial::avp::value_as<Time>(avp.value);
    BOOST_CHECK_EQUAL(value1.size(), 4);
    BOOST_CHECK_EQUAL(time_to_string(value1->to_unix()), "2024-05-21 08:01:09");

    auto value2 = diameter::serial::avp::value_as<Unsigned32>(avp.value);
    BOOST_CHECK_EQUAL(value1->value(), *value2);

    auto value4 = diameter::serial::avp::value_as<Integer32>(avp.value);
    BOOST_CHECK_EQUAL(value1->value(), *value4);
}

BOOST_AUTO_TEST_SUITE_END()

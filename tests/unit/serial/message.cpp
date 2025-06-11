#include <boost/test/unit_test.hpp>

#include <iostream>
#include <iomanip>
#include <vector>

#include <netpacker/error.h>

#include <diameter/message/avp/avp.h>
#include <diameter/serial/serial.h>

#include "../../helpers/from_hex.h"

using namespace diameter::message;

BOOST_AUTO_TEST_SUITE(msg)

BOOST_AUTO_TEST_CASE(decode)
{

    auto data = std::vector<uint8_t>{
        0x01,                     //Version
              0x00, 0x00, 0x40,   //Message Length
        0x80,                     //Command Flags
              0x00, 0x01, 0x01,   //Command Code
        0x00, 0x00, 0x01, 0x00,   //Application-ID
        0x00, 0x00, 0x00, 0x01,   //Hop-by-Hop Identifier
        0x00, 0x00, 0x00, 0x01,   //End-to-End Identifier

        // AVP1
        0x00, 0x00, 0x00, 0x01,   //Code
        0b10000000,               //Flags
              0x00, 0x00, 0x10,   //Length
        0x00, 0x00, 0x00, 0x01,   //VendorID
        0x00, 0x00, 0x00, 0x01,   //Data

        // AVP2
        0x00, 0x00, 0x00, 0x01,   //Code
        0b10000000,               //Flags
              0x00, 0x00, 0x0f,   //Length
        0x00, 0x00, 0x00, 0x01,   //VendorID
        0x00, 0x00, 0x01, 0x00,   //Data

        // AVP3
        0x00, 0x00, 0x00, 0x01,   //Code
        0b00000000,               //Flags
              0x00, 0x00, 0x0c,   //Length
        0x00, 0x00, 0x01, 0x00    //Data
    };

    auto pos = data.begin();
    auto msg = netpacker::get<Message>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(msg.header.version, header::ProtocolVersionV::V01);
    BOOST_CHECK_EQUAL(msg.header.length, 64);
    BOOST_CHECK_EQUAL(msg.header.command_flags[header::CommandFlag::Request], true);
    BOOST_CHECK_EQUAL(msg.header.command_flags[header::CommandFlag::Proxiable], false);
    BOOST_CHECK_EQUAL(msg.header.command_flags[header::CommandFlag::Error], false);
    BOOST_CHECK_EQUAL(msg.header.command_flags[header::CommandFlag::Retransmitted], false);
    BOOST_CHECK_EQUAL(msg.header.command_code, 257);
    BOOST_CHECK_EQUAL(msg.header.application_id, 256);
    BOOST_CHECK_EQUAL(msg.header.hop_by_hop, 1);
    BOOST_CHECK_EQUAL(msg.header.end_to_end, 1);

    BOOST_CHECK_EQUAL(msg.avps.size(), 3);
    for (const auto& avp : msg.avps) {
        BOOST_CHECK_EQUAL(avp.code, 1);
    }

    auto data2 = std::vector<uint8_t>(msg.size());
    netpacker::put(data2.begin(), data2.end(), msg);

    BOOST_CHECK_EQUAL_COLLECTIONS(data2.begin(), data2.end(), data.begin(), data.end());
}

BOOST_AUTO_TEST_CASE(decode_real_message)
{
    std::string hexed = \
        "010000C080000101000000006AD1D314" \
        "77287404000001084000003274657374" \
        "686F73742E6570632E6D6E633030302E" \
        "6D63633030302E336770706E6574776F" \
        "726B2E6F726700000000012840000029" \
        "6570632E6D6E633030302E6D63633030" \
        "302E336770706E6574776F726B2E6F72" \
        "67000000000001014000000E00017F00" \
        "000100000000010A4000000C000028AF" \
        "0000010D000000164578616D706C6550" \
        "726F647563740000000001024000000C" \
        "00000004000001094000000C000028AF";

    std::vector<uint8_t> data;
    load_from_hex(hexed, data);

    auto pos = data.begin();
    auto msg = netpacker::get<Message>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(msg.header.version, header::ProtocolVersionV::V01);
    BOOST_CHECK_EQUAL(msg.header.length, 192);
    BOOST_CHECK_EQUAL(msg.header.command_flags[header::CommandFlag::Request], true);
    BOOST_CHECK_EQUAL(msg.header.command_flags[header::CommandFlag::Proxiable], false);
    BOOST_CHECK_EQUAL(msg.header.command_flags[header::CommandFlag::Error], false);
    BOOST_CHECK_EQUAL(msg.header.command_flags[header::CommandFlag::Retransmitted], false);
    BOOST_CHECK_EQUAL(msg.header.command_code, 257);
    BOOST_CHECK_EQUAL(msg.header.application_id, 0);
    BOOST_CHECK_EQUAL(msg.header.hop_by_hop, 0x6ad1d314);
    BOOST_CHECK_EQUAL(msg.header.end_to_end, 0x77287404);
    BOOST_CHECK_EQUAL(msg.avps.size(), 7);

    for (const auto& avp : msg.avps) {
        if (avp.code == 264) {
            auto value = diameter::serial::avp::value_as<avp::DiameterIdentity>(avp.value);
            BOOST_CHECK_EQUAL(value->validate(), true);
            BOOST_CHECK_EQUAL(value->value(), "testhost.epc.mnc000.mcc000.3gppnetwork.org");
            BOOST_CHECK_EQUAL(value.size(), 42);
        }
        if (avp.code == 296) {
            auto value = diameter::serial::avp::value_as<avp::DiameterIdentity>(avp.value);
            BOOST_CHECK_EQUAL(value->validate(), true);
            BOOST_CHECK_EQUAL(value->value(), "epc.mnc000.mcc000.3gppnetwork.org");
            BOOST_CHECK_EQUAL(value.size(), 33);
        }
        if (avp.code == 257) {
            auto value = diameter::serial::avp::value_as<avp::Address>(avp.value);
            BOOST_CHECK_EQUAL(value->validate(), true);
            BOOST_CHECK_EQUAL(value->address_string(), "127.0.0.1");
            BOOST_CHECK_EQUAL(value.size(), 6);
        }
        if (avp.code == 266) {
            auto value = diameter::serial::avp::value_as<avp::Unsigned32>(avp.value);
            BOOST_CHECK_EQUAL(*value, 10415);
            BOOST_CHECK_EQUAL(value.data(), 10415);
            BOOST_CHECK_EQUAL(value.size(), 4);
        }
        if (avp.code == 269) {
            auto value = diameter::serial::avp::value_as<avp::UTF8String>(avp.value);
            BOOST_CHECK_EQUAL(*value, "ExampleProduct");
            BOOST_CHECK_EQUAL(value.data(), "ExampleProduct");
            BOOST_CHECK_EQUAL(value.size(), 14);
        }
        if (avp.code == 258) {
            auto value = diameter::serial::avp::value_as<avp::Unsigned32>(avp.value);
            BOOST_CHECK_EQUAL(*value, 4);
            BOOST_CHECK_EQUAL(value.data(), 4);
            BOOST_CHECK_EQUAL(value.size(), 4);
        }
        if (avp.code == 265) {
            auto value = diameter::serial::avp::value_as<avp::Unsigned32>(avp.value);
            BOOST_CHECK_EQUAL(*value, 10415);
            BOOST_CHECK_EQUAL(value.data(), 10415);
            BOOST_CHECK_EQUAL(value.size(), 4);
        }
    }
}

BOOST_AUTO_TEST_CASE(encode_message)
{
    Message m {
        {
            header::ProtocolVersion{header::ProtocolVersionV::V01},
            header::MessageLength{0},
            header::CommandFlags{0x80},
            header::CommandCode{257},
            header::ApplicationId{0},
            header::HopByHopIdentifier{0x6ad1d314},
            header::EndToEndIdentifier{0x77287404}
        },
        std::list<avp::AVP>{
            avp::AVP{264, avp::Flags{avp::Flag::Mandatory}, std::nullopt, avp::DiameterIdentity("testhost.epc.mnc000.mcc000.3gppnetwork.org")},
            avp::AVP{296, avp::Flags{avp::Flag::Mandatory}, std::nullopt, avp::DiameterIdentity("epc.mnc000.mcc000.3gppnetwork.org")},
            avp::AVP{257, avp::Flags{avp::Flag::Mandatory}, std::nullopt, avp::Address("127.0.0.1")},
            avp::AVP{266, avp::Flags{avp::Flag::Mandatory}, std::nullopt, avp::Unsigned32(uint32_t{10415})},
            avp::AVP{269, avp::Flags{}, std::nullopt, avp::UTF8String("ExampleProduct")},
            avp::AVP{258, avp::Flags{avp::Flag::Mandatory}, std::nullopt, avp::Unsigned32(uint32_t{4})},
            avp::AVP{265, avp::Flags{avp::Flag::Mandatory}, std::nullopt, avp::Unsigned32(uint32_t{10415})}
        }
    };

    auto data = std::vector<uint8_t>(m.size());
    netpacker::put(data.begin(), data.end(), m);

    std::string hexed = \
        "010000C080000101000000006AD1D314" \
        "77287404000001084000003274657374" \
        "686F73742E6570632E6D6E633030302E" \
        "6D63633030302E336770706E6574776F" \
        "726B2E6F726700000000012840000029" \
        "6570632E6D6E633030302E6D63633030" \
        "302E336770706E6574776F726B2E6F72" \
        "67000000000001014000000E00017F00" \
        "000100000000010A4000000C000028AF" \
        "0000010D000000164578616D706C6550" \
        "726F647563740000000001024000000C" \
        "00000004000001094000000C000028AF";

    std::vector<uint8_t> expected_data;
    load_from_hex(hexed, expected_data);

    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_SUITE_END()

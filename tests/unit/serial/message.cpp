#include <boost/test/unit_test.hpp>

#include <iostream>
#include <iomanip>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <variant>

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

namespace {

std::vector<uint8_t> message_bytes(uint32_t declared_length,
    std::initializer_list<uint8_t> body = {})
{
    // Build wire fixtures independently of the serializer under test.
    std::vector<uint8_t> data{
        0x01, static_cast<uint8_t>(declared_length >> 16),
        static_cast<uint8_t>(declared_length >> 8), static_cast<uint8_t>(declared_length),
        0x80, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    data.insert(data.end(), body.begin(), body.end());
    return data;
}

Message one_avp_message()
{
    return {{1, 28, header::CommandFlags{0x80}, 257, 0, 0x01234567, 0x89abcdef},
        {avp::AVP{1, avp::Flags{}, std::nullopt, avp::OctetString{}}}};
}

void check_encoded_length_is_consistent_or_rejected(Message msg)
{
    // Either rejection or normalization is acceptable for a stale cached length.
    const auto expected_length = static_cast<uint32_t>(20 + msg.avps.front().size());
    std::vector<uint8_t> data(expected_length, 0xa5);
    try {
        const auto end = netpacker::put(data.begin(), data.end(), msg);
        const uint32_t encoded_length = (static_cast<uint32_t>(data[1]) << 16)
            | (static_cast<uint32_t>(data[2]) << 8) | data[3];
        BOOST_CHECK_EQUAL(encoded_length, expected_length);
        BOOST_CHECK(end == data.end());
    }
    catch (const diameter::serial::Exception&) {
        // A domain error prevents an inconsistent frame from being sent.
    }
}

} // namespace

BOOST_AUTO_TEST_SUITE(serial_message_boundaries)

BOOST_AUTO_TEST_CASE(decode_header_only_message)
{
    auto data = message_bytes(20);
    auto pos = data.begin();
    auto decoded = netpacker::get<Message>(pos, data.end());
    BOOST_CHECK(decoded.avps.empty());
    BOOST_CHECK_EQUAL(decoded.header.length, 20);
    BOOST_CHECK(pos == data.end());
}

BOOST_AUTO_TEST_CASE(decode_empty_and_opaque_avps_preserves_order)
{
    auto data = message_bytes(40, {
        0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x08,
        0xff, 0xff, 0xff, 0xfd, 0x00, 0x00, 0x00, 0x0b,
        0x00, 0xff, 0x80, 0x00});
    auto pos = data.begin();
    auto decoded = netpacker::get<Message>(pos, data.end());
    BOOST_REQUIRE_EQUAL(decoded.avps.size(), 2);
    auto avp_it = decoded.avps.begin();
    BOOST_CHECK_EQUAL(avp_it->code, 0xfffffffeU);
    BOOST_REQUIRE(std::holds_alternative<avp::OctetString>(avp_it->value));
    BOOST_CHECK(std::get<avp::OctetString>(avp_it->value)->empty());
    ++avp_it;
    BOOST_CHECK_EQUAL(avp_it->code, 0xfffffffdU);
    const auto& payload = *std::get<avp::OctetString>(avp_it->value);
    const std::vector<uint8_t> expected{0x00, 0xff, 0x80};
    BOOST_CHECK_EQUAL_COLLECTIONS(payload.begin(), payload.end(),
        expected.begin(), expected.end());
    BOOST_CHECK(pos == data.end());
}

BOOST_AUTO_TEST_CASE(decode_consecutive_messages_consumes_exactly_one_frame)
{
    auto first = message_bytes(20);
    auto second = message_bytes(32, {
        0x00, 0x00, 0x01, 0x0a, 0x40, 0x00, 0x00, 0x0c,
        0x00, 0x00, 0x28, 0xaf});
    auto third = message_bytes(20);
    auto data = first;
    data.insert(data.end(), second.begin(), second.end());
    data.insert(data.end(), third.begin(), third.end());
    auto pos = data.begin();
    const auto first_message = netpacker::get<Message>(pos, data.end());
    BOOST_CHECK(first_message.avps.empty());
    BOOST_CHECK_EQUAL(std::distance(data.begin(), pos), 20);
    const auto second_message = netpacker::get<Message>(pos, data.end());
    BOOST_REQUIRE_EQUAL(second_message.avps.size(), 1);
    BOOST_CHECK_EQUAL(second_message.avps.front().code, 266);
    BOOST_CHECK_EQUAL(std::distance(data.begin(), pos), 52);
    const auto third_message = netpacker::get<Message>(pos, data.end());
    BOOST_CHECK(third_message.avps.empty());
    BOOST_CHECK(pos == data.end());
}

BOOST_AUTO_TEST_CASE(decode_rejects_every_truncated_frame)
{
    auto data = message_bytes(32, {
        0x00, 0x00, 0x01, 0x0a, 0x40, 0x00, 0x00, 0x0c,
        0x00, 0x00, 0x28, 0xaf});
    for (std::size_t length = 0; length < data.size(); ++length) {
        BOOST_TEST_CONTEXT("available bytes=" << length) {
            auto pos = data.begin();
            auto last = data.begin() + static_cast<std::ptrdiff_t>(length);
            BOOST_CHECK_THROW(netpacker::get<Message>(pos, last), std::exception);
            BOOST_CHECK(pos <= last);
        }
    }
}

BOOST_AUTO_TEST_CASE(decode_rejects_avp_crossing_declared_frame_boundary)
{
    auto data = message_bytes(28, {
        0x00, 0x00, 0x01, 0x0a, 0x40, 0x00, 0x00, 0x0c,
        0x00, 0x00, 0x28, 0xaf});
    auto pos = data.begin();
    BOOST_CHECK_THROW(netpacker::get<Message>(pos, data.end()), std::exception);
    BOOST_CHECK(pos <= data.begin() + 28);
}

BOOST_AUTO_TEST_CASE(decode_does_not_use_following_frame_to_finish_an_avp)
{
    auto data = message_bytes(28, {
        0x00, 0x00, 0x01, 0x0a, 0x40, 0x00, 0x00, 0x0c});
    const auto next = message_bytes(20);
    data.insert(data.end(), next.begin(), next.end());
    auto pos = data.begin();
    BOOST_CHECK_THROW(netpacker::get<Message>(pos, data.end()), std::exception);
    BOOST_CHECK(pos <= data.begin() + 28);
    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin() + 28, data.end(), next.begin(), next.end());
}

BOOST_AUTO_TEST_CASE(decode_rejects_partial_avp_header_inside_frame)
{
    auto data = message_bytes(24, {
        0x00, 0x00, 0x01, 0x0a, 0x00, 0x00, 0x00, 0x08});
    auto pos = data.begin();
    BOOST_CHECK_THROW(netpacker::get<Message>(pos, data.end()), std::exception);
    BOOST_CHECK(pos <= data.begin() + 24);
}

BOOST_AUTO_TEST_CASE(decode_rejects_padding_crossing_declared_frame_boundary)
{
    auto data = message_bytes(28, {
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x09,
        0x7f, 0x00, 0x00, 0x00});
    auto pos = data.begin();
    BOOST_CHECK_THROW(netpacker::get<Message>(pos, data.end()), std::exception);
    BOOST_CHECK(pos <= data.begin() + 28);
}

BOOST_AUTO_TEST_CASE(decode_rejects_truncated_avp_padding)
{
    auto data = message_bytes(32, {
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x09, 0x7f});
    auto pos = data.begin();
    BOOST_CHECK_THROW(netpacker::get<Message>(pos, data.end()), std::exception);
    BOOST_CHECK(pos <= data.end());
}

BOOST_AUTO_TEST_CASE(encode_header_only_message)
{
    Message msg{{1, 20, header::CommandFlags{0x80}, 257, 0,
                    0x01234567, 0x89abcdef}, {}};
    std::vector<uint8_t> data(20, 0xa5);
    const auto end = netpacker::put(data.begin(), data.end(), msg);
    const auto expected = message_bytes(20);
    BOOST_CHECK(end == data.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(encode_empty_avp_preserves_output_guards)
{
    const auto msg = one_avp_message();
    std::vector<uint8_t> data(32, 0xa5);
    const auto end = netpacker::put(data.begin() + 2, data.end() - 2, msg);
    const auto expected = message_bytes(28, {
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08});
    BOOST_CHECK(end == data.end() - 2);
    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin() + 2, end, expected.begin(), expected.end());
    BOOST_CHECK_EQUAL(data.front(), 0xa5);
    BOOST_CHECK_EQUAL(data[1], 0xa5);
    BOOST_CHECK_EQUAL(data[30], 0xa5);
    BOOST_CHECK_EQUAL(data.back(), 0xa5);
}

BOOST_AUTO_TEST_CASE(encode_rejects_every_truncated_output_range)
{
    const auto msg = one_avp_message();
    for (std::size_t length = 0; length < 28; ++length) {
        BOOST_TEST_CONTEXT("output bytes=" << length) {
            std::vector<uint8_t> data(length + 1, 0xa5);
            BOOST_CHECK_THROW(netpacker::put(data.begin(), data.end() - 1, msg),
                netpacker::BufferOverflow);
            BOOST_CHECK_EQUAL(data.back(), 0xa5);
        }
    }
}

BOOST_AUTO_TEST_CASE(encode_does_not_emit_zero_or_stale_message_length)
{
    for (uint32_t length : {0U, 20U, 32U}) {
        BOOST_TEST_CONTEXT("cached length=" << length) {
            auto msg = one_avp_message();
            msg.header.length = length;
            check_encoded_length_is_consistent_or_rejected(std::move(msg));
        }
    }
}

BOOST_AUTO_TEST_CASE(encode_detects_length_stale_after_avp_mutation)
{
    auto msg = one_avp_message();
    msg.size();
    msg.avps.front().value = avp::Unsigned32{0xdeadbeef};
    check_encoded_length_is_consistent_or_rejected(std::move(msg));
}

BOOST_AUTO_TEST_SUITE_END()

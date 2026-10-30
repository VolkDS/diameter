#include <boost/test/unit_test.hpp>

#include <iostream>
#include <iomanip>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>

#include <netpacker/error.h>

#include <diameter/serial/error.h>
#include <diameter/serial/header/header.h>

using namespace diameter;

BOOST_AUTO_TEST_SUITE(header)

BOOST_AUTO_TEST_CASE(decode)
{
    auto data = std::vector<uint8_t>{
        0x01,                     //Version
              0x00, 0x00, 0x14,   //Message Length
        0x80,                     //Command Flags
              0x00, 0x01, 0x01,   //Command Code
        0x00, 0x00, 0x01, 0x00,   //Application-ID
        0x00, 0x00, 0x00, 0x01,   //Hop-by-Hop Identifier
        0x00, 0x00, 0x00, 0x01    //End-to-End Identifier
    };

    auto pos = data.begin();
    auto header = netpacker::get<message::header::Header>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(header.version, message::header::ProtocolVersionV::V01);
    BOOST_CHECK_EQUAL(header.length, 20);
    BOOST_CHECK_EQUAL(header.command_flags[message::header::CommandFlag::Request], true);
    BOOST_CHECK_EQUAL(header.command_flags[message::header::CommandFlag::Proxiable], false);
    BOOST_CHECK_EQUAL(header.command_flags[message::header::CommandFlag::Error], false);
    BOOST_CHECK_EQUAL(header.command_flags[message::header::CommandFlag::Retransmitted], false);
    BOOST_CHECK_EQUAL(header.command_code, 257);
    BOOST_CHECK_EQUAL(header.application_id, 256);
    BOOST_CHECK_EQUAL(header.hop_by_hop, 1);
    BOOST_CHECK_EQUAL(header.end_to_end, 1);

    auto data2 = std::vector<uint8_t>(header.size());
    netpacker::put(data2.begin(), data2.end(), header);

    BOOST_CHECK_EQUAL(data2.size(), data.size());

    for (size_t i = 0; i < data2.size(); i++) {
        BOOST_CHECK_EQUAL(data2[i], data[i]);
    }
}

BOOST_AUTO_TEST_CASE(decode_null)
{
    auto data = std::vector<uint8_t>{
        0x01,                     //Version
              0x00, 0x00, 0x14,   //Message Length
        0x00,                     //Command Flags
              0x00, 0x00, 0x00,   //Command Code
        0x00, 0x00, 0x00, 0x00,   //Application-ID
        0x00, 0x00, 0x00, 0x00,   //Hop-by-Hop Identifier
        0x00, 0x00, 0x00, 0x00    //End-to-End Identifier
    };

    auto pos = data.begin();
    auto header = netpacker::get<message::header::Header>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(header.version, message::header::ProtocolVersionV::V01);
    BOOST_CHECK_EQUAL(header.length, 20);
    BOOST_CHECK_EQUAL(header.command_flags[message::header::CommandFlag::Request], false);
    BOOST_CHECK_EQUAL(header.command_flags[message::header::CommandFlag::Proxiable], false);
    BOOST_CHECK_EQUAL(header.command_flags[message::header::CommandFlag::Error], false);
    BOOST_CHECK_EQUAL(header.command_flags[message::header::CommandFlag::Retransmitted], false);
    BOOST_CHECK_EQUAL(header.command_code, 0);
    BOOST_CHECK_EQUAL(header.application_id, 0);
    BOOST_CHECK_EQUAL(header.hop_by_hop, 0);
    BOOST_CHECK_EQUAL(header.end_to_end, 0);
}

BOOST_AUTO_TEST_CASE(decode_full)
{
    auto data = std::vector<uint8_t>{
        0x01,                     //Version
              0x00, 0x00, 0x14,   //Message Length
        0xd0,                     //Request, Proxiable, Retransmitted
              0xff, 0xff, 0xff,   //Command Code
        0xff, 0xff, 0xff, 0xff,   //Application-ID
        0xff, 0xff, 0xff, 0xff,   //Hop-by-Hop Identifier
        0xff, 0xff, 0xff, 0xff    //End-to-End Identifier
    };

    auto pos = data.begin();
    auto header = netpacker::get<message::header::Header>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(header.version, message::header::ProtocolVersionV::V01);
    BOOST_CHECK_EQUAL(header.length, 20);
    BOOST_CHECK_EQUAL(header.command_flags[message::header::CommandFlag::Request], true);
    BOOST_CHECK_EQUAL(header.command_flags[message::header::CommandFlag::Proxiable], true);
    BOOST_CHECK_EQUAL(header.command_flags[message::header::CommandFlag::Error], false);
    BOOST_CHECK_EQUAL(header.command_flags[message::header::CommandFlag::Retransmitted], true);
    BOOST_CHECK_EQUAL(header.command_code, 0x00ffffff);
    BOOST_CHECK_EQUAL(header.application_id, 0xffffffff);
    BOOST_CHECK_EQUAL(header.hop_by_hop, 0xffffffff);
    BOOST_CHECK_EQUAL(header.end_to_end, 0xffffffff);
}

BOOST_AUTO_TEST_CASE(decode_invalid_version)
{
    auto data = std::vector<uint8_t>{
        0x02,                     //Version
              0x00, 0x00, 0x14,   //Message Length
        0x00,                     //Command Flags
              0x00, 0x00, 0x00,   //Command Code
        0x00, 0x00, 0x00, 0x00,   //Application-ID
        0x00, 0x00, 0x00, 0x00,   //Hop-by-Hop Identifier
        0x00, 0x00, 0x00, 0x00    //End-to-End Identifier
    };

    auto pos = data.begin();
    BOOST_CHECK_THROW(netpacker::get<message::header::Header>(pos, data.end()), serial::InvalidProtocolVersion);
}

BOOST_AUTO_TEST_CASE(decode_invalid_length)
{
    auto data = std::vector<uint8_t>{
        0x01,                     //Version
              0x00, 0x00, 0x10,   //Message Length
        0x00,                     //Command Flags
              0x00, 0x00, 0x00,   //Command Code
        0x00, 0x00, 0x00, 0x00,   //Application-ID
        0x00, 0x00, 0x00, 0x00,   //Hop-by-Hop Identifier
        0x00, 0x00, 0x00, 0x00    //End-to-End Identifier
    };

    auto pos = data.begin();
    BOOST_CHECK_THROW(netpacker::get<message::header::Header>(pos, data.end()), serial::InvalidMessageLength);
}

BOOST_AUTO_TEST_CASE(decode_end_of_buffer)
{
    auto data1 = std::vector<uint8_t>{
        0x01,                     //Version
              0x00, 0x00, 0x14,   //Message Length
        0x00,                     //Command Flags
              0x00, 0x00, 0x00,   //Command Code
        0x00, 0x00, 0x00, 0x00,   //Application-ID
        0x00, 0x00, 0x00, 0x00    //Hop-by-Hop Identifier
        //0x00, 0x00, 0x00, 0x00    //End-to-End Identifier
    };

    auto data2 = std::vector<uint8_t>{
        0x01,                     //Version
              0x00, 0x00, 0x14,   //Message Length
        0x00,                     //Command Flags
              0x00, 0x00, 0x00,   //Command Code
        0x00, 0x00, 0x00, 0x00,   //Application-ID
        0x00, 0x00, 0x00, 0x00,   //Hop-by-Hop Identifier
        0x00, 0x00, 0x00          //End-to-End Identifier
    };

    auto data3 = std::vector<uint8_t>{
        0x01,                     //Version
              0x00, 0x00, 0x14,   //Message Length
        0x00,                     //Command Flags
              0x00, 0x00          //Command Code
    };

    auto pos1 = data1.begin();
    BOOST_CHECK_THROW(netpacker::get<message::header::Header>(pos1, data1.end()), netpacker::EndOfBuffer);

    auto pos2 = data2.begin();
    BOOST_CHECK_THROW(netpacker::get<message::header::Header>(pos2, data2.end()), netpacker::EndOfBuffer);

    auto pos3 = data3.begin();
    BOOST_CHECK_THROW(netpacker::get<message::header::Header>(pos3, data3.end()), netpacker::EndOfBuffer);
}

BOOST_AUTO_TEST_CASE(encode)
{
    auto expected_data = std::vector<uint8_t>{
        0x01,                     //Version
              0x00, 0x00, 0x14,   //Message Length
        0x00,                     //Command Flags
              0x00, 0x00, 0x00,   //Command Code
        0x00, 0x00, 0x00, 0x00,   //Application-ID
        0x00, 0x00, 0x00, 0x00,   //Hop-by-Hop Identifier
        0x00, 0x00, 0x00, 0x00    //End-to-End Identifier
    };

    message::header::Header header{};

    header.version = message::header::ProtocolVersionV::V01;
    header.length = header.size();
    auto data = std::vector<uint8_t>(header.length);
    netpacker::put(data.begin(), data.end(), header);

    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(encode_overflow)
{
    message::header::Header header{};

    header.version = message::header::ProtocolVersionV::V01;
    header.length = header.size();
    auto data = std::vector<uint8_t>(header.length - 1);
    BOOST_CHECK_THROW(netpacker::put(data.begin(), data.end(), header), netpacker::BufferOverflow);
}

BOOST_AUTO_TEST_SUITE_END()

namespace {

std::vector<uint8_t> valid_header_bytes()
{
    return {0x01, 0x00, 0x00, 0x14, 0x80, 0x00, 0x01, 0x01,
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xfe, 0xdc, 0xba, 0x98};
}

message::header::Header valid_header()
{
    return {1, 20, message::header::CommandFlags{0x80}, 257,
        0x01234567, 0x89abcdef, 0xfedcba98};
}

} // namespace

BOOST_AUTO_TEST_SUITE(serial_header_boundaries)

BOOST_AUTO_TEST_CASE(decode_rejects_every_truncated_header)
{
    auto data = valid_header_bytes();
    for (std::size_t length = 0; length < data.size(); ++length) {
        BOOST_TEST_CONTEXT("available bytes=" << length) {
            auto pos = data.begin();
            auto last = data.begin() + static_cast<std::ptrdiff_t>(length);
            BOOST_CHECK_THROW(netpacker::get<message::header::Header>(pos, last),
                netpacker::EndOfBuffer);
            BOOST_CHECK(pos <= last);
        }
    }
}

BOOST_AUTO_TEST_CASE(decode_rejects_all_lengths_below_header_size)
{
    auto data = valid_header_bytes();
    for (uint8_t length = 0; length < 20; ++length) {
        BOOST_TEST_CONTEXT("declared length=" << static_cast<unsigned>(length)) {
            data[3] = length;
            auto pos = data.begin();
            BOOST_CHECK_THROW(netpacker::get<message::header::Header>(pos, data.end()),
                serial::InvalidMessageLength);
        }
    }
}

BOOST_AUTO_TEST_CASE(decode_rejects_unaligned_message_lengths)
{
    // RFC 6733 Section 3 includes AVP padding in Message Length.
    for (uint8_t length : {uint8_t{21}, uint8_t{22}, uint8_t{23}, uint8_t{25}}) {
        BOOST_TEST_CONTEXT("declared length=" << static_cast<unsigned>(length)) {
            auto data = valid_header_bytes();
            data[3] = length;
            auto pos = data.begin();
            BOOST_CHECK_THROW(netpacker::get<message::header::Header>(pos, data.end()),
                serial::InvalidMessageLength);
        }
    }
}

BOOST_AUTO_TEST_CASE(decode_rejects_unsupported_versions)
{
    for (uint8_t version : {uint8_t{0}, uint8_t{2}, uint8_t{255}}) {
        BOOST_TEST_CONTEXT("version=" << static_cast<unsigned>(version)) {
            auto data = valid_header_bytes();
            data[0] = version;
            auto pos = data.begin();
            BOOST_CHECK_THROW(netpacker::get<message::header::Header>(pos, data.end()),
                serial::InvalidProtocolVersion);
        }
    }
}

BOOST_AUTO_TEST_CASE(decode_header_does_not_require_message_body)
{
    auto data = valid_header_bytes();
    data[1] = 0xff;
    data[2] = 0xff;
    data[3] = 0xfc;
    auto pos = data.begin();
    auto decoded = netpacker::get<message::header::Header>(pos, data.end());
    BOOST_CHECK_EQUAL(decoded.length, 0x00fffffcU);
    BOOST_CHECK(pos == data.end());
}

BOOST_AUTO_TEST_CASE(decode_from_offset_consumes_only_header)
{
    auto header_bytes = valid_header_bytes();
    std::vector<uint8_t> data(3, 0xa5);
    data.insert(data.end(), header_bytes.begin(), header_bytes.end());
    data.insert(data.end(), 5, 0x5a);
    auto pos = data.begin() + 3;
    auto decoded = netpacker::get<message::header::Header>(pos, data.end());
    BOOST_CHECK_EQUAL(decoded.application_id, 0x01234567U);
    BOOST_CHECK_EQUAL(decoded.hop_by_hop, 0x89abcdefU);
    BOOST_CHECK_EQUAL(decoded.end_to_end, 0xfedcba98U);
    BOOST_REQUIRE_EQUAL(std::distance(data.begin(), pos), 23);
    BOOST_CHECK_EQUAL(*pos, 0x5a);
}

BOOST_AUTO_TEST_CASE(encode_network_byte_order_and_subrange_guards)
{
    const auto header = valid_header();
    const auto expected = valid_header_bytes();
    std::vector<uint8_t> data(25, 0xa5);
    const auto end = netpacker::put(data.begin() + 2, data.begin() + 22, header);
    BOOST_CHECK(end == data.begin() + 22);
    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin() + 2, end, expected.begin(), expected.end());
    BOOST_CHECK_EQUAL(data[0], 0xa5);
    BOOST_CHECK_EQUAL(data[1], 0xa5);
    BOOST_CHECK_EQUAL(data[22], 0xa5);
    BOOST_CHECK_EQUAL(data[23], 0xa5);
    BOOST_CHECK_EQUAL(data[24], 0xa5);
}

BOOST_AUTO_TEST_CASE(encode_rejects_every_short_output_buffer)
{
    const auto header = valid_header();
    for (std::size_t length = 0; length < 20; ++length) {
        BOOST_TEST_CONTEXT("output bytes=" << length) {
            std::vector<uint8_t> data(length + 1, 0xa5);
            BOOST_CHECK_THROW(netpacker::put(data.begin(), data.end() - 1, header),
                netpacker::BufferOverflow);
            BOOST_CHECK_EQUAL(data.back(), 0xa5);
        }
    }
}

BOOST_AUTO_TEST_CASE(encode_preserves_largest_representable_fields)
{
    auto header = valid_header();
    header.length = 0x00fffffc;
    header.command_code = 0x00ffffff;
    header.application_id = 0xffffffff;
    header.hop_by_hop = 0xffffffff;
    header.end_to_end = 0xffffffff;
    std::vector<uint8_t> data(20);
    BOOST_CHECK(netpacker::put(data.begin(), data.end(), header) == data.end());
    const std::vector<uint8_t> expected{
        0x01, 0xff, 0xff, 0xfc, 0x80, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff};
    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(encode_rejects_length_that_does_not_fit_24_bits)
{
    auto header = valid_header();
    header.length = 0x01000000;
    std::vector<uint8_t> data(20);
    BOOST_CHECK_THROW(netpacker::put(data.begin(), data.end(), header), std::exception);
}

BOOST_AUTO_TEST_CASE(encode_rejects_command_code_that_does_not_fit_24_bits)
{
    auto header = valid_header();
    header.command_code = 0x01000000;
    std::vector<uint8_t> data(20);
    BOOST_CHECK_THROW(netpacker::put(data.begin(), data.end(), header), std::exception);
}

BOOST_AUTO_TEST_CASE(encode_rejects_invalid_version)
{
    auto header = valid_header();
    header.version = 0;
    std::vector<uint8_t> data(20);
    BOOST_CHECK_THROW(netpacker::put(data.begin(), data.end(), header),
        serial::InvalidProtocolVersion);
}

BOOST_AUTO_TEST_CASE(encode_rejects_invalid_message_lengths)
{
    for (uint32_t length : {0U, 16U, 19U, 21U, 22U, 23U}) {
        BOOST_TEST_CONTEXT("declared length=" << length) {
            auto header = valid_header();
            header.length = length;
            std::vector<uint8_t> data(20);
            BOOST_CHECK_THROW(netpacker::put(data.begin(), data.end(), header),
                serial::InvalidMessageLength);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

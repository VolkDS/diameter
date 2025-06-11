#include <boost/test/unit_test.hpp>

#include <iostream>
#include <iomanip>
#include <vector>

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
        0xff,                     //Command Flags
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
    BOOST_CHECK_EQUAL(header.command_flags[message::header::CommandFlag::Error], true);
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
        0x00,                     //Version
              0x00, 0x00, 0x14,   //Message Length
        0x00,                     //Command Flags
              0x00, 0x00, 0x00,   //Command Code
        0x00, 0x00, 0x00, 0x00,   //Application-ID
        0x00, 0x00, 0x00, 0x00,   //Hop-by-Hop Identifier
        0x00, 0x00, 0x00, 0x00    //End-to-End Identifier
    };

    message::header::Header header{};

    header.length = header.size();
    auto data = std::vector<uint8_t>(header.length);
    netpacker::put(data.begin(), data.end(), header);

    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(encode_overflow)
{
    message::header::Header header{};

    header.length = header.size();
    auto data = std::vector<uint8_t>(header.length - 1);
    BOOST_CHECK_THROW(netpacker::put(data.begin(), data.end(), header), netpacker::BufferOverflow);
}

BOOST_AUTO_TEST_SUITE_END()

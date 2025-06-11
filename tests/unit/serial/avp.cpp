#include <boost/test/unit_test.hpp>

#include <iomanip>
#include <iostream>
#include <vector>

#include <netpacker/error.h>

#include <diameter/serial/avp/avp.h>
#include <diameter/serial/error.h>

#include "../../helpers/from_hex.h"
#include "../../helpers/string_to_time.h"

using namespace diameter;

BOOST_AUTO_TEST_SUITE(avp)

BOOST_AUTO_TEST_CASE(decode)
{
    auto data = std::vector<uint8_t>{
        0x00, 0x00, 0x00, 0x01,   //Code
        0b10000000,               //Flags
              0x00, 0x00, 0x10,   //Length
        0x00, 0x00, 0x00, 0x01,   //VendorID
        0x00, 0x00, 0x00, 0x01    //Data
    };

    auto pos = data.begin();
    auto avp = netpacker::get<message::avp::AVP>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(avp.code, 1);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::VendorSpecific], true);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Mandatory], false);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Protected], false);
    BOOST_CHECK_EQUAL(avp.length(), 16);
    BOOST_CHECK_EQUAL(avp.size(), 16);
    BOOST_CHECK_EQUAL(avp.vendor_id.has_value(), true);
    BOOST_CHECK_EQUAL(avp.vendor_id.value(), 1);

    auto value = serial::avp::value_as<message::avp::OctetString>(avp.value);
    BOOST_CHECK_EQUAL(value.size(), 4);

    auto& raw_data = *value;
    for (size_t i = 0; i < value.size(); i++) {
        BOOST_CHECK_EQUAL(raw_data[i], data[i + 12]);
    }
}

BOOST_AUTO_TEST_CASE(decode_padding)
{
    auto data = std::vector<uint8_t>{
        0x00, 0x00, 0x00, 0x01,   //Code
        0b10000000,               //Flags
              0x00, 0x00, 0x0f,   //Length
        0x00, 0x00, 0x00, 0x01,   //VendorID
        0x00, 0x00, 0x01, 0x00    //Data
    };

    auto pos = data.begin();
    auto avp = netpacker::get<message::avp::AVP>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(avp.code, 1);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::VendorSpecific], true);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Mandatory], false);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Protected], false);
    BOOST_CHECK_EQUAL(avp.length(), 15);
    BOOST_CHECK_EQUAL(avp.size(), 16);
    BOOST_CHECK_EQUAL(avp.vendor_id.has_value(), true);
    BOOST_CHECK_EQUAL(avp.vendor_id.value(), 1);

    auto value = serial::avp::value_as<message::avp::OctetString>(avp.value);
    BOOST_CHECK_EQUAL(value.size(), 3);

    auto& raw_data = *value;
    for (size_t i = 0; i < value.size(); i++) {
        BOOST_CHECK_EQUAL(raw_data[i], data[i + 12]);
    }
}

BOOST_AUTO_TEST_CASE(decode_invalid_cast)
{
    auto data = std::vector<uint8_t>{
        0x00, 0x00, 0x03, 0xe8,   //Code
        0b01000000,               //Flags
              0x00, 0x00, 0x10,   //Length
        0xFF, 0xFF, 0xFF, 0xFF,   //Data
        0xFF, 0xFF, 0xFF, 0xFE
    };

    auto pos = data.begin();
    auto avp = netpacker::get<message::avp::AVP>(pos, data.end());

    BOOST_CHECK_THROW(diameter::serial::avp::value_as<message::avp::Integer32>(avp.value), diameter::serial::InvalidAvpValueCast);
}

BOOST_AUTO_TEST_CASE(decode_octet_string_avp)
{
    // AVP: ECGI(2517) l=19 f=VM- vnd=TGPP val=04f17001cc1105
    //     AVP Code: 2517 ECGI
    //     AVP Flags: 0xc0, Vendor-Specific: Set, Mandatory: Set
    //     AVP Length: 19
    //     AVP Vendor Id: 3GPP (10415)
    //     ECGI: 04f17001cc1105
    //     Padding: 00
    std::string hexed = "000009d5c0000013000028af04f17001cc110500";
    std::vector<uint8_t> data;
    load_from_hex(hexed, data);

    auto pos = data.begin();
    auto avp = netpacker::get<message::avp::AVP>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(avp.code, 2517);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::VendorSpecific], true);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Mandatory], true);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Protected], false);
    BOOST_CHECK_EQUAL(avp.length(), 19);
    BOOST_CHECK_EQUAL(avp.size(), hexed.size() / 2);
    BOOST_CHECK_EQUAL(avp.vendor_id.has_value(), true);
    BOOST_CHECK_EQUAL(avp.vendor_id.value(), 10415);

    auto value = diameter::serial::avp::value_as<message::avp::OctetString>(avp.value);
    BOOST_CHECK_EQUAL(value.size(), 7);

    auto& raw_data = *value;
    std::vector<uint8_t> expected_data = {0x04, 0xf1, 0x70, 0x01, 0xcc, 0x11, 0x05};
    BOOST_CHECK_EQUAL_COLLECTIONS(raw_data.begin(), raw_data.end(), expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(decode_integer32_avp)
{
    auto data = std::vector<uint8_t>{
        0x00, 0x00, 0x03, 0xe8,   //Code
        0b01000000,               //Flags
              0x00, 0x00, 0x0c,   //Length
        0xFF, 0xFF, 0xFF, 0xFE    //Data
    };

    auto pos = data.begin();
    auto avp = netpacker::get<message::avp::AVP>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(avp.code, 1000);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::VendorSpecific], false);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Mandatory], true);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Protected], false);
    BOOST_CHECK_EQUAL(avp.length(), 12);
    BOOST_CHECK_EQUAL(avp.size(), data.size());
    BOOST_CHECK_EQUAL(avp.vendor_id.has_value(), false);

    auto value = serial::avp::value_as<message::avp::Integer32>(avp.value);
    BOOST_CHECK_EQUAL(value.data(), -2);
    BOOST_CHECK_EQUAL(value.size(), 4);
}

BOOST_AUTO_TEST_CASE(decode_integer64_avp)
{
    auto data = std::vector<uint8_t>{
        0x00, 0x00, 0x03, 0xe8,   //Code
        0b01000000,               //Flags
              0x00, 0x00, 0x10,   //Length
        0xFF, 0xFF, 0xFF, 0xFE,   //Data
        0xFF, 0xFF, 0xFF, 0xFE
    };

    auto pos = data.begin();
    auto avp = netpacker::get<message::avp::AVP>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(avp.code, 1000);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::VendorSpecific], false);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Mandatory], true);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Protected], false);
    BOOST_CHECK_EQUAL(avp.length(), 16);
    BOOST_CHECK_EQUAL(avp.size(), data.size());
    BOOST_CHECK_EQUAL(avp.vendor_id.has_value(), false);

    auto value = serial::avp::value_as<message::avp::Integer64>(avp.value);
    BOOST_CHECK_EQUAL(value.data(), -4294967298);
    BOOST_CHECK_EQUAL(value.size(), 8);
}

BOOST_AUTO_TEST_CASE(decode_unsigned32_avp)
{
    // AVP: IDR-Flags(1490) l=16 f=VM- vnd=TGPP val=11
    //     AVP Code: 1490 IDR-Flags
    //     AVP Flags: 0xc0, Vendor-Specific: Set, Mandatory: Set
    //     AVP Length: 16
    //     AVP Vendor Id: 3GPP (10415)
    //     IDR Flags: 0x0000000b
    std::string hexed = "000005d2c0000010000028af0000000b";
    std::vector<uint8_t> data;
    load_from_hex(hexed, data);

    auto pos = data.begin();
    auto avp = netpacker::get<message::avp::AVP>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(avp.code, 1490);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::VendorSpecific], true);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Mandatory], true);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Protected], false);
    BOOST_CHECK_EQUAL(avp.length(), 16);
    BOOST_CHECK_EQUAL(avp.size(), hexed.size() / 2);
    BOOST_CHECK_EQUAL(avp.vendor_id.has_value(), true);
    BOOST_CHECK_EQUAL(avp.vendor_id.value(), 10415);

    auto value = serial::avp::value_as<message::avp::Unsigned32>(avp.value);
    BOOST_CHECK_EQUAL(value.data(), 11);
    BOOST_CHECK_EQUAL(value.size(), 4);
}

BOOST_AUTO_TEST_CASE(decode_unsigned64_avp)
{
    auto data = std::vector<uint8_t>{
        0x00, 0x00, 0x03, 0xe8,   //Code
        0b01000000,               //Flags
              0x00, 0x00, 0x10,   //Length
        0x00, 0x00, 0x00, 0xFE,   //Data
        0xFF, 0xFF, 0xFF, 0xFF
    };

    auto pos = data.begin();
    auto avp = netpacker::get<message::avp::AVP>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(avp.code, 1000);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::VendorSpecific], false);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Mandatory], true);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Protected], false);
    BOOST_CHECK_EQUAL(avp.length(), 16);
    BOOST_CHECK_EQUAL(avp.size(), data.size());
    BOOST_CHECK_EQUAL(avp.vendor_id.has_value(), false);

    auto value = serial::avp::value_as<message::avp::Unsigned64>(avp.value);
    BOOST_CHECK_EQUAL(value.data(), 1095216660479);
    BOOST_CHECK_EQUAL(value.size(), 8);
}

BOOST_AUTO_TEST_CASE(decode_float32_avp)
{
    std::string hexed = "000005d2c0000010000028af3fc00000";
    std::vector<uint8_t> data;
    load_from_hex(hexed, data);

    auto pos = data.begin();
    auto avp = netpacker::get<message::avp::AVP>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(avp.code, 1490);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::VendorSpecific], true);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Mandatory], true);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Protected], false);
    BOOST_CHECK_EQUAL(avp.length(), 16);
    BOOST_CHECK_EQUAL(avp.size(), hexed.size() / 2);
    BOOST_CHECK_EQUAL(avp.vendor_id.has_value(), true);
    BOOST_CHECK_EQUAL(avp.vendor_id.value(), 10415);

    auto value = serial::avp::value_as<message::avp::Float32>(avp.value);
    BOOST_CHECK_EQUAL(value.data(), 1.5);
    BOOST_CHECK_EQUAL(value.size(), 4);
}

BOOST_AUTO_TEST_CASE(decode_float64_avp)
{
    std::string hexed = "000005d2c0000014000028af3ff8000000000000";
    std::vector<uint8_t> data;
    load_from_hex(hexed, data);

    auto pos = data.begin();
    auto avp = netpacker::get<message::avp::AVP>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(avp.code, 1490);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::VendorSpecific], true);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Mandatory], true);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Protected], false);
    BOOST_CHECK_EQUAL(avp.length(), 20);
    BOOST_CHECK_EQUAL(avp.size(), hexed.size() / 2);
    BOOST_CHECK_EQUAL(avp.vendor_id.has_value(), true);
    BOOST_CHECK_EQUAL(avp.vendor_id.value(), 10415);

    auto value = serial::avp::value_as<message::avp::Float64>(avp.value);
    BOOST_CHECK_EQUAL(value.data(), 1.5);
    BOOST_CHECK_EQUAL(value.size(), 8);
}

BOOST_AUTO_TEST_CASE(decode_grouped_avp)
{
}

BOOST_AUTO_TEST_CASE(decode_address_avp)
{
    std::string hexed = "000001014000000E0001C00002010000";
    std::vector<uint8_t> data;
    load_from_hex(hexed, data);

    auto pos = data.begin();
    auto avp = netpacker::get<message::avp::AVP>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(avp.code, 257);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::VendorSpecific], false);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Mandatory], true);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Protected], false);
    BOOST_CHECK_EQUAL(avp.length(), 14);
    BOOST_CHECK_EQUAL(avp.size(), hexed.size() / 2);
    BOOST_CHECK_EQUAL(avp.vendor_id.has_value(), false);

    auto value = serial::avp::value_as<message::avp::Address>(avp.value);
    BOOST_CHECK_EQUAL(value->validate(), true);
    BOOST_CHECK_EQUAL(value->address_family(), 1);
    BOOST_CHECK_EQUAL(value.size(), 6);
    BOOST_CHECK_EQUAL(value->is_ipv4(), true);
    BOOST_CHECK_EQUAL(value->is_ipv6(), false);

    auto expected_data = std::vector<uint8_t>{0xC0, 0x00, 0x02, 0x01};
    auto address = value->value();

    BOOST_CHECK_EQUAL_COLLECTIONS(address.begin(), address.end(), expected_data.begin(), expected_data.end());
    BOOST_CHECK_EQUAL(value->address_string(), "192.0.2.1");
}

BOOST_AUTO_TEST_CASE(decode_time_avp)
{
    // AVP: SIP-Request-Timestamp(834) l=16 f=VM- vnd=TGPP val=Jun 30, 2017 06:00:03.000000000 UTC
    //     AVP Code: 834 SIP-Request-Timestamp
    //     AVP Flags: 0xc0, Vendor-Specific: Set, Mandatory: Set
    //     AVP Length: 16
    //     AVP Vendor Id: 3GPP (10415)
    //     SIP-Request-Timestamp: Jun 30, 2017 06:00:03.000000000 UTC
    std::string hexed = "00000342c0000010000028afdd006763";
    std::vector<uint8_t> data;
    load_from_hex(hexed, data);

    auto pos = data.begin();
    auto avp = netpacker::get<message::avp::AVP>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(avp.code, 834);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::VendorSpecific], true);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Mandatory], true);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Protected], false);
    BOOST_CHECK_EQUAL(avp.length(), 16);
    BOOST_CHECK_EQUAL(avp.size(), hexed.size() / 2);
    BOOST_CHECK_EQUAL(avp.vendor_id.has_value(), true);
    BOOST_CHECK_EQUAL(avp.vendor_id.value(), 10415);

    auto value = serial::avp::value_as<message::avp::Time>(avp.value);
    BOOST_CHECK_EQUAL(time_to_string(value->to_unix()), "2017-06-30 06:00:03");
    BOOST_CHECK_EQUAL(value.size(), 4);
}

BOOST_AUTO_TEST_CASE(decode_ip_filter_rule_avp)
{
    std::string hexed = "0000010F400000377065726D697420696E2069702066726F6D203139322E3136382E312E302F323420746F2031302E302E302E312F333200";
    std::vector<uint8_t> data;
    load_from_hex(hexed, data);

    auto pos = data.begin();
    auto avp = netpacker::get<message::avp::AVP>(pos, data.end());

    BOOST_CHECK_EQUAL(std::distance(pos, data.end()), 0);
    BOOST_CHECK_EQUAL(avp.code, 271);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::VendorSpecific], false);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Mandatory], true);
    BOOST_CHECK_EQUAL(avp.flags[message::avp::Flag::Protected], false);
    BOOST_CHECK_EQUAL(avp.length(), 55);
    BOOST_CHECK_EQUAL(avp.size(), hexed.size() / 2);
    BOOST_CHECK_EQUAL(avp.vendor_id.has_value(), false);

    auto value = serial::avp::value_as<message::avp::IPFilterRule>(avp.value);
    BOOST_CHECK_EQUAL(value->value(), "permit in ip from 192.168.1.0/24 to 10.0.0.1/32");
    BOOST_CHECK_EQUAL(value.size(), 47);
}

BOOST_AUTO_TEST_CASE(encode_octet_string_avp)
{
    auto avp = message::avp::AVP{
        1000,
        message::avp::Flags{message::avp::Flag::Mandatory} | message::avp::Flags{message::avp::Flag::Protected},
        std::nullopt,
        message::avp::OctetString(message::avp::OctetString::value_type{0x01, 0x02, 0x03, 0x04})
    };

    //avp.code = 1000;
    //avp.flags.set(message::avp::Flag::VendorSpecific, false);
    //avp.flags.set(message::avp::Flag::Mandatory, true);
    //avp.flags.set(message::avp::Flag::Protected, true);
    //avp.value = message::avp::OctetString(message::avp::OctetString::value_type{0x01, 0x02, 0x03, 0x04});

    auto data = std::vector<uint8_t>(avp.size());
    netpacker::put(data.begin(), data.end(), avp);

    auto expected_data = std::vector<uint8_t>{
        0x00, 0x00, 0x03, 0xe8,   //Code
        0b01100000,               //Flags
              0x00, 0x00, 0x0c,   //Length
        0x01, 0x02, 0x03, 0x04    //Data
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(encode_integer32_avp)
{
    auto avp = message::avp::AVP{
        1000,
        message::avp::Flags{message::avp::Flag::Mandatory},
        std::nullopt,
        message::avp::Integer32(1)
    };

    auto data = std::vector<uint8_t>(avp.size());
    netpacker::put(data.begin(), data.end(), avp);

    auto expected_data = std::vector<uint8_t>{
        0x00, 0x00, 0x03, 0xe8,   //Code
        0b01000000,               //Flags
              0x00, 0x00, 0x0c,   //Length
        0x00, 0x00, 0x00, 0x01    //Data
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(encode_integer64_avp)
{
    auto avp = message::avp::AVP{
        1000,
        message::avp::Flags{message::avp::Flag::Mandatory},
        std::nullopt,
        message::avp::Integer64(-2)
    };

    auto data = std::vector<uint8_t>(avp.size());
    netpacker::put(data.begin(), data.end(), avp);

    auto expected_data = std::vector<uint8_t>{
        0x00, 0x00, 0x03, 0xe8,   //Code
        0b01000000,               //Flags
              0x00, 0x00, 0x10,   //Length
        0xFF, 0xFF, 0xFF, 0xFF,   //Data
        0xFF, 0xFF, 0xFF, 0xFE
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(encode_unsigned32_avp)
{
    auto avp = message::avp::AVP{
        1000,
        message::avp::Flags{message::avp::Flag::Mandatory},
        std::nullopt,
        message::avp::Unsigned32(static_cast<uint32_t>(4'294'967'295))
    };

    auto data = std::vector<uint8_t>(avp.size());
    netpacker::put(data.begin(), data.end(), avp);

    auto expected_data = std::vector<uint8_t>{
        0x00, 0x00, 0x03, 0xe8,   //Code
        0b01000000,               //Flags
              0x00, 0x00, 0x0c,   //Length
        0xFF, 0xFF, 0xFF, 0xFF    //Data
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(encode_unsigned64_avp)
{
    auto avp = message::avp::AVP{
        1000,
        message::avp::Flags{message::avp::Flag::Mandatory},
        std::nullopt,
        message::avp::Unsigned64(18'446'744'073'709'551'615ULL)
    };

    auto data = std::vector<uint8_t>(avp.size());
    netpacker::put(data.begin(), data.end(), avp);

    auto expected_data = std::vector<uint8_t>{
        0x00, 0x00, 0x03, 0xe8,   //Code
        0b01000000,               //Flags
              0x00, 0x00, 0x10,   //Length
        0xFF, 0xFF, 0xFF, 0xFF,   //Data
        0xFF, 0xFF, 0xFF, 0xFF
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(encode_float32_avp)
{
    auto avp = message::avp::AVP{
        1000,
        message::avp::Flags{message::avp::Flag::Mandatory},
        std::nullopt,
        message::avp::Float32(static_cast<float>(1.5))
    };

    auto data = std::vector<uint8_t>(avp.size());
    netpacker::put(data.begin(), data.end(), avp);

    auto expected_data = std::vector<uint8_t>{
        0x00, 0x00, 0x03, 0xe8,   //Code
        0b01000000,               //Flags
              0x00, 0x00, 0x0c,   //Length
        0x3f, 0xc0, 0x00, 0x00    //Data
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(encode_float64_avp)
{
    auto avp = message::avp::AVP{
        1000,
        message::avp::Flags{message::avp::Flag::Mandatory},
        std::nullopt,
        message::avp::Float64(1.5)
    };

    auto data = std::vector<uint8_t>(avp.size());
    netpacker::put(data.begin(), data.end(), avp);

    auto expected_data = std::vector<uint8_t>{
        0x00, 0x00, 0x03, 0xe8,   //Code
        0b01000000,               //Flags
              0x00, 0x00, 0x10,   //Length
        0x3f, 0xf8, 0x00, 0x00,   //Data
        0x00, 0x00, 0x00, 0x00
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(encode_grouped_avp)
{
    // Example-AVP ::= < AVP Header: 999999 >
    //                 { Origin-Host }
    //               1*{ Session-Id }
    //                *[ AVP ]

    std::vector<uint8_t> data1;
    std::string data_str1 = "2163bc1d0ad82371f6bc09484133c3f09ad74a0dd5346d54195a7cf0b352cabc881839a4fdcf44bc1769e26ffacfebc77a4c122fb4f99284c5f70b48f58503aef45c543c2d6943f82d5930f2b7c1da640f476f0e9c9572a50db8ea6e51e1c2c7bdf8bb43dc995144b8dbe297ac739493946803e1cee3e15d9b765008a1b2acf4ac777c80041d72c01e691cf751dbf86e85f509f3988e5875dc90511926841f00f0e29a6d1ddc1a842289d440268681e052b30fb638045f7779c1d873c784f054f688f5001559ecff64865ef975f3e60d2fd7966b8c7f92";
    load_from_hex(data_str1, data1);

    std::vector<uint8_t> data2;
    std::string data_str2 = "fe19da5802acd98b07a5b86cb4d5d03fffef0314ab9ef1ad0b67111ff3b90a057fe29620bf3585fd2dd9fcc38ce62f6cc208c6163c008f4258d1bc88b817694a74ccad3ec69269461b14b2e7a4c111fb239e33714da207983f58c41d018d56fe938f3cbf089aac12a912a2f0d1923a9390e5f789cb2e5067d3427475e49968f841";
    load_from_hex(data_str2, data2);

    auto avp = message::avp::AVP{
        999999,
        message::avp::Flags{message::avp::Flag::Mandatory},
        std::nullopt,
        message::avp::Grouped(message::avp::Grouped::value_type{
            message::avp::AVP{
                264,
                message::avp::Flags{message::avp::Flag::Mandatory},
                std::nullopt,
                message::avp::DiameterIdentity("example.com")
            },
            message::avp::AVP{
                263,
                message::avp::Flags{message::avp::Flag::Mandatory},
                std::nullopt,
                message::avp::UTF8String("grump.example.com:33041;23432;893;0AF3B81")
            },
            message::avp::AVP{
                262,
                message::avp::Flags{message::avp::Flag::Mandatory},
                std::nullopt,
                message::avp::UTF8String("grump.example.com:33054;23561;2358;0AF3B82")
            },
            message::avp::AVP{
                8341,
                message::avp::Flags{message::avp::Flag::Mandatory},
                std::nullopt,
                message::avp::OctetString(data1)
            },
            message::avp::AVP{
                15930,
                message::avp::Flags{message::avp::Flag::Mandatory},
                std::nullopt,
                message::avp::OctetString(data2)
            }
        })
    };

    auto data = std::vector<uint8_t>(avp.size());
    netpacker::put(data.begin(), data.end(), avp);

    BOOST_CHECK_EQUAL(avp.length(), 496);
    auto value = serial::avp::value_as<message::avp::Grouped>(avp.value);
    for (const auto& a : *value) {
        if (a.code == 264) {
            BOOST_CHECK_EQUAL(a.length(), 19);
        }
        if (a.code == 263) {
            BOOST_CHECK_EQUAL(a.length(), 49);
        }
        if (a.code == 262) {
            BOOST_CHECK_EQUAL(a.length(), 50);
        }
        if (a.code == 8341) {
            BOOST_CHECK_EQUAL(a.length(), 223);
        }
        if (a.code == 15930) {
            BOOST_CHECK_EQUAL(a.length(), 137);
        }
    }

    std::string hexed = \
        "000F423F400001F00000010840000013" \
        "6578616D706C652E636F6D0000000107" \
        "400000316772756D702E6578616D706C" \
        "652E636F6D3A33333034313B32333433" \
        "323B3839333B30414633423831000000" \
        "00000106400000326772756D702E6578" \
        "616D706C652E636F6D3A33333035343B" \
        "32333536313B323335383B3041463342" \
        "3832000000002095400000DF" + data_str1 + "00" \
        "00003E3A40000089" + data_str2 + "000000";

    std::vector<uint8_t> expected_data;
    load_from_hex(hexed, expected_data);
    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(encode_address_avp)
{
    auto avp = message::avp::AVP{
        1000,
        message::avp::Flags{message::avp::Flag::Mandatory},
        std::nullopt,
        //message::avp::Address(message::avp::value::AddressFamilyV::IPv4, std::vector<uint8_t>{0xC0, 0x00, 0x02, 0x01})
        //message::avp::Address(message::avp::value::AddressFamilyV::IPv4, "192.0.2.1")
        message::avp::Address("192.0.2.1")
    };

    auto data = std::vector<uint8_t>(avp.size());
    netpacker::put(data.begin(), data.end(), avp);

    auto expected_data = std::vector<uint8_t>{
        0x00, 0x00, 0x03, 0xe8,   //Code
        0b01000000,               //Flags
              0x00, 0x00, 0x0e,   //Length
        0x00, 0x01, 0xC0, 0x00,   //Data
        0x02, 0x01, 0x00, 0x00
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(encode_time_avp)
{
    auto avp = message::avp::AVP{
        1000,
        message::avp::Flags{message::avp::Flag::Mandatory},
        std::nullopt,
        message::avp::Time(string_to_time("1999-12-31 00:00:00"))
    };

    auto data = std::vector<uint8_t>(avp.size());
    netpacker::put(data.begin(), data.end(), avp);

    auto expected_data = std::vector<uint8_t>{
        0x00, 0x00, 0x03, 0xe8,   //Code
        0b01000000,               //Flags
              0x00, 0x00, 0x0c,   //Length
        0xbc, 0x16, 0x70, 0x80    //Data
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(encode_utf8_string_avp)
{
    auto avp = message::avp::AVP{
        1000,
        message::avp::Flags{message::avp::Flag::Mandatory},
        std::nullopt,
        message::avp::UTF8String("ExampleProduct")
    };

    auto data = std::vector<uint8_t>(avp.size());
    netpacker::put(data.begin(), data.end(), avp);

    std::string hexed = "000003E8400000164578616D706C6550726F647563740000";
    std::vector<uint8_t> expected_data;
    load_from_hex(hexed, expected_data);
    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), expected_data.begin(), expected_data.end());
}

BOOST_AUTO_TEST_CASE(encode_diameter_identity_avp)
{
}

BOOST_AUTO_TEST_CASE(encode_diameter_uri_avp)
{
}

BOOST_AUTO_TEST_CASE(encode_enumerated_avp)
{
}

BOOST_AUTO_TEST_CASE(encode_ipfilter_rule_avp)
{
}

BOOST_AUTO_TEST_SUITE_END()

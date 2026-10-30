#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>

#include <cmath>
#include <limits>

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

    const auto empty = OctetString();
    const auto from_iterators = OctetString(data.begin(), data.end());
    const auto from_vector = OctetString(data);
    BOOST_CHECK(empty->empty());
    BOOST_CHECK_EQUAL(empty.size(), 0);
    BOOST_CHECK_EQUAL_COLLECTIONS(from_iterators->begin(), from_iterators->end(),
        data.begin(), data.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(from_vector->begin(), from_vector->end(),
        data.begin(), data.end());
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
    BOOST_CHECK_EQUAL(value1->value(), static_cast<uint32_t>(*value4));
}

using FixedWidthValues = boost::mpl::list<Integer32, Integer64, Unsigned32, Unsigned64,
    Float32, Float64, Time, Enumerated>;
using NumericValues = boost::mpl::list<Integer32, Integer64, Unsigned32, Unsigned64,
    Float32, Float64>;

BOOST_AUTO_TEST_CASE_TEMPLATE(rejects_incorrect_fixed_width, T, FixedWidthValues)
{
    constexpr std::size_t width = std::is_same_v<T, Integer64>
        || std::is_same_v<T, Unsigned64> || std::is_same_v<T, Float64> ? 8 : 4;
    for (std::size_t length = 0; length <= 9; ++length) {
        if (length == width) {
            continue;
        }
        BOOST_TEST_CONTEXT("OctetString length " << length << ", expected " << width) {
            const Value raw = OctetString(std::vector<uint8_t>(length, 0));
            BOOST_CHECK_THROW(diameter::serial::avp::value_as<T>(raw),
                diameter::serial::InvalidAvpValueCast);
        }
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(numeric_same_type_and_wire_round_trip, T, NumericValues)
{
    using Raw = typename T::value_type;
    const Raw values[]{Raw{0}, Raw{1}, std::numeric_limits<Raw>::lowest(),
        std::numeric_limits<Raw>::max()};
    for (const auto original : values) {
        BOOST_TEST_CONTEXT("Numeric value " << original) {
            const Value typed = T(original);
            const auto copied = diameter::serial::avp::value_as<T>(typed);
            BOOST_CHECK_EQUAL(*copied, original);
            const auto wire = diameter::serial::avp::value_as<OctetString>(typed);
            BOOST_CHECK_EQUAL(wire.size(), sizeof(Raw));
            const auto decoded = diameter::serial::avp::value_as<T>(Value(wire));
            BOOST_CHECK_EQUAL(*decoded, original);
        }
    }
}

BOOST_AUTO_TEST_CASE(signed_integer_network_order_boundaries)
{
    const Value minimum32 = Integer32(std::numeric_limits<int32_t>::min());
    const Value maximum32 = Integer32(std::numeric_limits<int32_t>::max());
    const Value minimum64 = Integer64(std::numeric_limits<int64_t>::min());
    const Value maximum64 = Integer64(std::numeric_limits<int64_t>::max());
    const std::vector<uint8_t> expected_min32{0x80, 0, 0, 0};
    const std::vector<uint8_t> expected_max32{0x7f, 0xff, 0xff, 0xff};
    const std::vector<uint8_t> expected_min64{0x80, 0, 0, 0, 0, 0, 0, 0};
    const std::vector<uint8_t> expected_max64{0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    const Value values[]{minimum32, maximum32, minimum64, maximum64};
    const std::vector<uint8_t> expected[]{expected_min32, expected_max32, expected_min64, expected_max64};
    for (std::size_t i = 0; i < 4; ++i) {
        const auto wire = diameter::serial::avp::value_as<OctetString>(values[i]);
        BOOST_CHECK_EQUAL_COLLECTIONS(wire->begin(), wire->end(), expected[i].begin(), expected[i].end());
    }
}

BOOST_AUTO_TEST_CASE(unsigned_integer_network_order_boundaries)
{
    const Value maximum32 = Unsigned32(std::numeric_limits<uint32_t>::max());
    const Value maximum64 = Unsigned64(std::numeric_limits<uint64_t>::max());
    const std::vector<uint8_t> expected32(4, 0xff);
    const std::vector<uint8_t> expected64(8, 0xff);
    const auto wire32 = diameter::serial::avp::value_as<OctetString>(maximum32);
    const auto wire64 = diameter::serial::avp::value_as<OctetString>(maximum64);
    BOOST_CHECK_EQUAL_COLLECTIONS(wire32->begin(), wire32->end(), expected32.begin(), expected32.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(wire64->begin(), wire64->end(), expected64.begin(), expected64.end());
}

BOOST_AUTO_TEST_CASE(float32_special_values_preserve_wire_bits)
{
    const std::vector<std::vector<uint8_t>> patterns{
        {0x80, 0, 0, 0}, {0x7f, 0x80, 0, 0}, {0xff, 0x80, 0, 0},
        {0x7f, 0xc0, 0, 1}, {0, 0, 0, 1}};
    for (const auto& pattern : patterns) {
        const auto typed = diameter::serial::avp::value_as<Float32>(Value(OctetString(pattern)));
        const auto wire = diameter::serial::avp::value_as<OctetString>(Value(typed));
        BOOST_CHECK_EQUAL_COLLECTIONS(wire->begin(), wire->end(), pattern.begin(), pattern.end());
    }
    const auto negative_zero = diameter::serial::avp::value_as<Float32>(
        Value(OctetString(patterns[0])));
    BOOST_CHECK_EQUAL(*negative_zero, 0.0F);
    BOOST_CHECK(std::signbit(*negative_zero));
}

BOOST_AUTO_TEST_CASE(float64_special_values_preserve_wire_bits)
{
    const std::vector<std::vector<uint8_t>> patterns{
        {0x80, 0, 0, 0, 0, 0, 0, 0}, {0x7f, 0xf0, 0, 0, 0, 0, 0, 0},
        {0xff, 0xf0, 0, 0, 0, 0, 0, 0}, {0x7f, 0xf8, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 1}};
    for (const auto& pattern : patterns) {
        const auto typed = diameter::serial::avp::value_as<Float64>(Value(OctetString(pattern)));
        const auto wire = diameter::serial::avp::value_as<OctetString>(Value(typed));
        BOOST_CHECK_EQUAL_COLLECTIONS(wire->begin(), wire->end(), pattern.begin(), pattern.end());
    }
    const auto negative_zero = diameter::serial::avp::value_as<Float64>(
        Value(OctetString(patterns[0])));
    BOOST_CHECK_EQUAL(*negative_zero, 0.0);
    BOOST_CHECK(std::signbit(*negative_zero));
}

BOOST_AUTO_TEST_CASE(utf8_string_preserves_multibyte_characters)
{
    const std::vector<uint8_t> bytes{0x41, 0xd0, 0x96};
    const Value raw = OctetString(bytes);
    const auto text = diameter::serial::avp::value_as<UTF8String>(raw);
    const std::string expected("A\xd0\x96", 3);
    BOOST_CHECK_EQUAL(*text, expected);
    const auto copy = diameter::serial::avp::value_as<UTF8String>(Value(text));
    BOOST_CHECK_EQUAL(*copy, expected);
    const auto wire = diameter::serial::avp::value_as<OctetString>(Value(copy));
    BOOST_CHECK_EQUAL_COLLECTIONS(wire->begin(), wire->end(), bytes.begin(), bytes.end());
}

BOOST_AUTO_TEST_CASE(octet_string_preserves_embedded_null)
{
    const std::vector<uint8_t> bytes{0x41, 0, 0xd0, 0x96};
    const Value raw = OctetString(bytes);
    const auto raw_copy = diameter::serial::avp::value_as<OctetString>(raw);
    BOOST_CHECK_EQUAL_COLLECTIONS(raw_copy->begin(), raw_copy->end(), bytes.begin(), bytes.end());
}

BOOST_AUTO_TEST_CASE(derived_values_same_type_conversion)
{
    const Value identity = DiameterIdentity("peer.example.com");
    const Value uri = DiameterURI("aaas://peer.example.com:5658;transport=tcp;protocol=diameter");
    const Value enumerated = Enumerated(-1);
    const Value rule = IPFilterRule("permit in ip from any to any");
    const Value time_value = Time(uint32_t{0xffffffff});
    const Value address = Address("192.0.2.1");
    BOOST_CHECK_EQUAL(diameter::serial::avp::value_as<DiameterIdentity>(identity)->value(),
        "peer.example.com");
    BOOST_CHECK_EQUAL(diameter::serial::avp::value_as<DiameterURI>(uri)->value(),
        "aaas://peer.example.com:5658;transport=tcp;protocol=diameter");
    BOOST_CHECK_EQUAL(diameter::serial::avp::value_as<Enumerated>(enumerated)->value(), -1);
    BOOST_CHECK_EQUAL(diameter::serial::avp::value_as<IPFilterRule>(rule)->value(),
        "permit in ip from any to any");
    BOOST_CHECK_EQUAL(diameter::serial::avp::value_as<Time>(time_value)->value(), 0xffffffffU);
    BOOST_CHECK_EQUAL(diameter::serial::avp::value_as<Address>(address)->address_string(), "192.0.2.1");
}

BOOST_AUTO_TEST_CASE(empty_string_and_grouped_conversions)
{
    const Value raw = OctetString{};
    BOOST_CHECK(diameter::serial::avp::value_as<UTF8String>(raw)->empty());
    BOOST_CHECK(diameter::serial::avp::value_as<DiameterIdentity>(raw)->value().empty());
    BOOST_CHECK(diameter::serial::avp::value_as<DiameterURI>(raw)->value().empty());
    BOOST_CHECK(diameter::serial::avp::value_as<IPFilterRule>(raw)->value().empty());
    BOOST_CHECK(diameter::serial::avp::value_as<Grouped>(raw)->empty());
    BOOST_CHECK(diameter::serial::avp::value_as<OctetString>(Value(Grouped{}))->empty());
    BOOST_CHECK(diameter::serial::avp::value_as<Grouped>(Value(Grouped{}))->empty());
}

BOOST_AUTO_TEST_CASE(unsupported_typed_conversions_throw)
{
    const Value integer = Integer32(1);
    BOOST_CHECK_THROW(diameter::serial::avp::value_as<Unsigned32>(integer),
        diameter::serial::InvalidAvpValueCast);
    BOOST_CHECK_THROW(diameter::serial::avp::value_as<Float32>(integer),
        diameter::serial::InvalidAvpValueCast);
    BOOST_CHECK_THROW(diameter::serial::avp::value_as<UTF8String>(integer),
        diameter::serial::InvalidAvpValueCast);
    const Value string = UTF8String("test");
    BOOST_CHECK_THROW(diameter::serial::avp::value_as<Integer32>(string),
        diameter::serial::InvalidAvpValueCast);
}

BOOST_AUTO_TEST_CASE(grouped_rejects_partial_child_header)
{
    for (std::size_t size = 1; size < 8; ++size) {
        BOOST_TEST_CONTEXT("Grouped data length " << size) {
            const Value raw = OctetString(std::vector<uint8_t>(size, 0));
            BOOST_CHECK_THROW(diameter::serial::avp::value_as<Grouped>(raw),
                netpacker::EndOfBuffer);
        }
    }
}

BOOST_AUTO_TEST_CASE(grouped_rejects_invalid_child_length)
{
    for (uint8_t length = 0; length < 8; ++length) {
        BOOST_TEST_CONTEXT("Child length " << static_cast<unsigned>(length)) {
            const Value raw = OctetString(std::vector<uint8_t>{0, 0, 0, 1, 0, 0, 0, length});
            BOOST_CHECK_THROW(diameter::serial::avp::value_as<Grouped>(raw),
                diameter::serial::InvalidAvpLength);
        }
    }
}

BOOST_AUTO_TEST_CASE(grouped_rejects_child_data_beyond_parent)
{
    const Value raw = OctetString(std::vector<uint8_t>{0, 0, 0, 1, 0, 0, 0, 12, 0});
    BOOST_CHECK_THROW(diameter::serial::avp::value_as<Grouped>(raw), netpacker::EndOfBuffer);
}

BOOST_AUTO_TEST_CASE(grouped_rejects_missing_child_padding)
{
    const Value raw = OctetString(std::vector<uint8_t>{0, 0, 0, 1, 0, 0, 0, 9, 0x42});
    BOOST_CHECK_THROW(diameter::serial::avp::value_as<Grouped>(raw), netpacker::EndOfBuffer);
}

BOOST_AUTO_TEST_CASE(grouped_rejects_trailing_partial_child)
{
    const Value raw = OctetString(std::vector<uint8_t>{0, 0, 0, 1, 0, 0, 0, 8, 0});
    BOOST_CHECK_THROW(diameter::serial::avp::value_as<Grouped>(raw), netpacker::EndOfBuffer);
}

BOOST_AUTO_TEST_CASE(grouped_nested_values_round_trip)
{
    const AVP leaf{3, Flags{}, std::nullopt, OctetString(std::vector<uint8_t>{0x42})};
    const AVP child{2, Flags{}, std::nullopt, Grouped(Grouped::value_type{leaf})};
    const Value value = Grouped(Grouped::value_type{child});
    const std::vector<uint8_t> expected{
        0, 0, 0, 2, 0, 0, 0, 20, 0, 0, 0, 3, 0, 0, 0, 9, 0x42, 0, 0, 0};
    const auto wire = diameter::serial::avp::value_as<OctetString>(value);
    BOOST_CHECK_EQUAL_COLLECTIONS(wire->begin(), wire->end(), expected.begin(), expected.end());
    const auto decoded = diameter::serial::avp::value_as<Grouped>(Value(wire));
    BOOST_REQUIRE_EQUAL(decoded->size(), 1);
    BOOST_CHECK_EQUAL(decoded->front().code, 2);
    const auto nested = diameter::serial::avp::value_as<Grouped>(decoded->front().value);
    BOOST_REQUIRE_EQUAL(nested->size(), 1);
    BOOST_CHECK_EQUAL(nested->front().code, 3);
    const auto payload = diameter::serial::avp::value_as<OctetString>(nested->front().value);
    BOOST_REQUIRE_EQUAL(payload->size(), 1);
    BOOST_CHECK_EQUAL(payload->front(), 0x42);
}

BOOST_AUTO_TEST_CASE(address_conversion_rejects_missing_family_bytes)
{
    for (std::size_t length = 0; length < 2; ++length) {
        const Value raw = OctetString(std::vector<uint8_t>(length, 0));
        BOOST_CHECK_THROW(diameter::serial::avp::value_as<Address>(raw), netpacker::EndOfBuffer);
    }
}

BOOST_AUTO_TEST_CASE(address_conversion_preserves_ipv6_bytes)
{
    const std::vector<uint8_t> expected{0, 2, 0x20, 1, 0x0d, 0xb8,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    auto address = diameter::serial::avp::value_as<Address>(Value(OctetString(expected)));
    BOOST_CHECK(address->validate());
    BOOST_CHECK(address->is_ipv6());
    BOOST_CHECK_EQUAL(address->address_string(), "2001:db8::1");
    const auto wire = diameter::serial::avp::value_as<OctetString>(Value(address));
    BOOST_CHECK_EQUAL_COLLECTIONS(wire->begin(), wire->end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(raw_value_decoder_consumes_only_requested_bytes)
{
    const std::vector<uint8_t> bytes{0, 0, 0, 42, 0xaa};
    auto pos = bytes.begin();
    const auto value = netpacker::get<Value>(pos, bytes.end(), 4);
    BOOST_CHECK_EQUAL(std::distance(bytes.begin(), pos), 4);
    BOOST_REQUIRE(pos != bytes.end());
    BOOST_CHECK_EQUAL(*pos, 0xaa);
    BOOST_CHECK_EQUAL(*diameter::serial::avp::value_as<Unsigned32>(value), 42U);
    BOOST_CHECK_THROW(netpacker::get<Value>(pos, bytes.end(), 2), netpacker::EndOfBuffer);
}

BOOST_AUTO_TEST_CASE(raw_value_encoder_preserves_surrounding_bytes)
{
    const Value value = Unsigned32(0x12345678U);
    std::vector<uint8_t> bytes(6, 0xaa);
    const auto end = netpacker::put(bytes.begin() + 1, bytes.end() - 1, value);
    BOOST_CHECK(end == bytes.end() - 1);
    const std::vector<uint8_t> expected{0xaa, 0x12, 0x34, 0x56, 0x78, 0xaa};
    BOOST_CHECK_EQUAL_COLLECTIONS(bytes.begin(), bytes.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(nested_grouped_rejects_grandchild_crossing_child_boundary)
{
    // The child's eight-byte data contains a grandchild declaring twelve bytes.
    const Value raw = OctetString(std::vector<uint8_t>{
        0, 0, 0, 1, 0, 0, 0, 16, 0, 0, 0, 2, 0, 0, 0, 12});
    const auto grouped = diameter::serial::avp::value_as<Grouped>(raw);
    BOOST_REQUIRE_EQUAL(grouped->size(), 1);
    BOOST_CHECK_THROW(diameter::serial::avp::value_as<Grouped>(grouped->front().value),
        netpacker::EndOfBuffer);
}

BOOST_AUTO_TEST_SUITE_END()

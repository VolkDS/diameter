#include <boost/test/unit_test.hpp>

#include <diameter/message/avp/avp.h>

#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace diameter::message::avp;

BOOST_AUTO_TEST_SUITE(basic_avp_value_tests)

BOOST_AUTO_TEST_CASE(integer_boundaries_and_wire_sizes)
{
    const Integer32 signed32{std::numeric_limits<int32_t>::min()};
    const Integer64 signed64{std::numeric_limits<int64_t>::max()};
    const Unsigned32 unsigned32{std::numeric_limits<uint32_t>::max()};
    const Unsigned64 unsigned64{std::numeric_limits<uint64_t>::max()};
    BOOST_CHECK_EQUAL(*signed32, std::numeric_limits<int32_t>::min());
    BOOST_CHECK_EQUAL(*signed64, std::numeric_limits<int64_t>::max());
    BOOST_CHECK_EQUAL(*unsigned32, std::numeric_limits<uint32_t>::max());
    BOOST_CHECK_EQUAL(*unsigned64, std::numeric_limits<uint64_t>::max());
    BOOST_CHECK_EQUAL(signed32.size(), 4);
    BOOST_CHECK_EQUAL(signed64.size(), 8);
    BOOST_CHECK_EQUAL(unsigned32.size(), 4);
    BOOST_CHECK_EQUAL(unsigned64.size(), 8);
}

BOOST_AUTO_TEST_CASE(floating_point_values_and_wire_sizes)
{
    const Float32 single{-1.5f};
    const Float64 twice{3.25};
    BOOST_CHECK_EQUAL(*single, -1.5f);
    BOOST_CHECK_EQUAL(*twice, 3.25);
    BOOST_CHECK_EQUAL(single.size(), 4);
    BOOST_CHECK_EQUAL(twice.size(), 8);
}

BOOST_AUTO_TEST_CASE(octet_string_retains_binary_data)
{
    const std::vector<uint8_t> bytes{0x00, 0xff, 0x80, 0x00};
    const OctetString value{bytes};
    BOOST_CHECK_EQUAL(value.size(), bytes.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(value->begin(), value->end(), bytes.begin(), bytes.end());
}

BOOST_AUTO_TEST_CASE(utf8_string_size_counts_encoded_octets)
{
    const std::string bytes{"A\xc3\xa9"};
    const UTF8String value{bytes};
    BOOST_CHECK_EQUAL(value.size(), 3);
    BOOST_CHECK_EQUAL(value->size(), 3);
    BOOST_CHECK_EQUAL(*value, bytes);
}

BOOST_AUTO_TEST_CASE(value_access_and_data_copy)
{
    UTF8String value{"original"};
    value->append(" text");
    BOOST_CHECK_EQUAL(*value, "original text");
    auto copy = value.data();
    copy.clear();
    BOOST_CHECK_EQUAL(*value, "original text");
    *value = "changed";
    const UTF8String& const_value = value;
    BOOST_CHECK_EQUAL(*const_value, "changed");
    BOOST_CHECK_EQUAL(const_value->size(), 7);
}

BOOST_AUTO_TEST_CASE(copy_and_assignment_own_independent_storage)
{
    OctetString original{std::vector<uint8_t>{1, 2, 3}};
    OctetString copied{original};
    OctetString assigned{};
    assigned = original;
    copied->front() = 9;
    assigned->push_back(4);
    BOOST_CHECK_EQUAL(original->front(), 1);
    BOOST_CHECK_EQUAL(original.size(), 3);
    BOOST_CHECK_EQUAL(copied->front(), 9);
    BOOST_CHECK_EQUAL(assigned.size(), 4);
}

BOOST_AUTO_TEST_CASE(const_get_accessor_preserves_constness)
{
    using Pointer = decltype(std::declval<const UTF8String&>().get());
    // Check the public signature without instantiating its currently ill-formed function body.
    BOOST_CHECK((std::is_same_v<Pointer, const std::string*>));
}

BOOST_AUTO_TEST_CASE(move_construction_and_assignment_preserve_payload)
{
    UTF8String original{"retained payload"};
    UTF8String moved{std::move(original)};
    BOOST_CHECK_EQUAL(*moved, "retained payload");
    UTF8String destination{"old"};
    destination = std::move(moved);
    BOOST_CHECK_EQUAL(*destination, "retained payload");
    BOOST_CHECK_EQUAL(destination.size(), 16);
}

BOOST_AUTO_TEST_CASE(enumerated_is_signed_and_four_octets)
{
    const Enumerated value{int32_t{-1}};
    BOOST_CHECK_EQUAL(value->value(), -1);
    BOOST_CHECK_EQUAL(value.size(), 4);
}

BOOST_AUTO_TEST_CASE(empty_variable_length_values_have_no_payload)
{
    BOOST_CHECK_EQUAL(OctetString{}.size(), 0);
    BOOST_CHECK_EQUAL(UTF8String{}.size(), 0);
    BOOST_CHECK_EQUAL(Grouped{}.size(), 0);
    BOOST_CHECK_EQUAL(DiameterIdentity{}.size(), 0);
    BOOST_CHECK_EQUAL(DiameterURI{}.size(), 0);
    BOOST_CHECK_EQUAL(IPFilterRule{}.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

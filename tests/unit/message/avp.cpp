#include <boost/test/unit_test.hpp>

#include <diameter/message/avp/avp.h>

#include <list>

using namespace diameter::message::avp;

BOOST_AUTO_TEST_SUITE(avp_tests)

BOOST_AUTO_TEST_CASE(default_constructor)
{
    AVP a{};

    BOOST_CHECK_EQUAL(a.code, Code{});
    BOOST_CHECK_EQUAL(a.flags[Flag::VendorSpecific], false);
    BOOST_CHECK_EQUAL(a.flags[Flag::Mandatory], false);
    BOOST_CHECK_EQUAL(a.flags[Flag::Protected], false);
    BOOST_CHECK_EQUAL(a.vendor_id.has_value(), false);
    BOOST_CHECK_EQUAL(a.value.index(), 0);
}

BOOST_AUTO_TEST_CASE(field_initialization)
{
    // clang-format off
    AVP a{
        Code{1000},
        Flags{Flag::VendorSpecific} | Flags{Flag::Mandatory},
        10415,
        Unsigned32(uint32_t{0})
    };
    // clang-format on

    BOOST_CHECK_EQUAL(a.code, Code{1000});
    BOOST_CHECK_EQUAL(a.flags[Flag::VendorSpecific], true);
    BOOST_CHECK_EQUAL(a.flags[Flag::Mandatory], true);
    BOOST_CHECK_EQUAL(a.flags[Flag::Protected], false);
    BOOST_CHECK_EQUAL(a.vendor_id.has_value(), true);
    BOOST_CHECK_EQUAL(a.vendor_id.value(), 10415);
}

BOOST_AUTO_TEST_CASE(size_calculation)
{
    AVP a{};

    BOOST_CHECK_EQUAL(a.size(), 8);
    BOOST_CHECK_EQUAL(a.length(), 8);

    a.flags.set(Flag::VendorSpecific);
    a.vendor_id = 10415;

    BOOST_CHECK_EQUAL(a.size(), 12);
    BOOST_CHECK_EQUAL(a.length(), 12);
}

BOOST_AUTO_TEST_CASE(padding_for_each_payload_remainder)
{
    for (uint32_t payload_size = 0; payload_size <= 9; ++payload_size) {
        BOOST_TEST_CONTEXT("payload size " << payload_size) {
            AVP value{1, Flags{}, std::nullopt,
                OctetString(std::vector<uint8_t>(payload_size, 0xff))};
            BOOST_CHECK_EQUAL(value.length(), 8 + payload_size);
            BOOST_CHECK_EQUAL(value.padding(), (4 - payload_size % 4) % 4);
            BOOST_CHECK_EQUAL(value.size(), (8 + payload_size + 3) / 4 * 4);

            value.flags.set(Flag::VendorSpecific);
            value.vendor_id = 10415;
            BOOST_CHECK_EQUAL(value.length(), 12 + payload_size);
            BOOST_CHECK_EQUAL(value.padding(), (4 - payload_size % 4) % 4);
            BOOST_CHECK_EQUAL(value.size(), (12 + payload_size + 3) / 4 * 4);
        }
    }
}

BOOST_AUTO_TEST_CASE(fixed_width_payload_lengths)
{
    const Value values[]{Integer32{-1}, Integer64{-1}, Unsigned32{1u}, Unsigned64{1u},
        Float32{1.0f}, Float64{1.0}, Enumerated{int32_t{-1}}, Time{uint32_t{0}}};
    const Length expected[]{12, 16, 12, 16, 12, 16, 12, 12};
    for (std::size_t i = 0; i < std::size(values); ++i) {
        const AVP value{1, Flags{}, std::nullopt, values[i]};
        BOOST_CHECK_EQUAL(value.length(), expected[i]);
        BOOST_CHECK_EQUAL(value.size(), expected[i]);
        BOOST_CHECK_EQUAL(value.padding(), 0);
    }
}

BOOST_AUTO_TEST_CASE(grouped_payload_includes_child_padding)
{
    const AVP first{1, Flags{}, std::nullopt, UTF8String{"a"}};
    const AVP second{2, Flags{Flag::VendorSpecific}, 10415, UTF8String{"bc"}};
    const Grouped children{std::list<AVP>{first, second}};
    const AVP parent{3, Flags{}, std::nullopt, children};
    BOOST_CHECK_EQUAL(first.length(), 9);
    BOOST_CHECK_EQUAL(first.size(), 12);
    BOOST_CHECK_EQUAL(second.length(), 14);
    BOOST_CHECK_EQUAL(second.size(), 16);
    BOOST_CHECK_EQUAL(children.size(), 28);
    BOOST_CHECK_EQUAL(parent.length(), 36);
    BOOST_CHECK_EQUAL(parent.size(), 36);
    BOOST_CHECK_EQUAL(parent.padding(), 0);
}

BOOST_AUTO_TEST_CASE(nested_and_empty_grouped_lengths)
{
    const AVP empty{1, Flags{}, std::nullopt, Grouped{}};
    const AVP leaf{2, Flags{}, std::nullopt, UTF8String{"abc"}};
    const AVP inner{3, Flags{}, std::nullopt, Grouped{std::list<AVP>{leaf, empty}}};
    const AVP outer{4, Flags{Flag::VendorSpecific}, 10415,
        Grouped{std::list<AVP>{inner}}};
    BOOST_CHECK_EQUAL(empty.length(), 8);
    BOOST_CHECK_EQUAL(inner.length(), 28);
    BOOST_CHECK_EQUAL(outer.length(), 40);
    BOOST_CHECK_EQUAL(outer.padding(), 0);
}

BOOST_AUTO_TEST_CASE(copy_preserves_vendor_and_deep_copies_grouped_children)
{
    AVP original{1000, Flags{Flag::VendorSpecific}, 10415,
        Grouped{std::list<AVP>{AVP{1, Flags{}, std::nullopt, UTF8String{"abc"}}}}};
    AVP copy{original};
    auto& copied_children = *std::get<Grouped>(copy.value);
    *std::get<UTF8String>(copied_children.front().value) = "modified";
    copy.vendor_id = 1;

    const auto& original_children = *std::get<Grouped>(original.value);
    BOOST_CHECK_EQUAL(*std::get<UTF8String>(original_children.front().value), "abc");
    BOOST_CHECK_EQUAL(*std::get<UTF8String>(copied_children.front().value), "modified");
    BOOST_CHECK_EQUAL(original.vendor_id.value(), 10415);
    BOOST_CHECK_EQUAL(copy.code, original.code);
    BOOST_CHECK_EQUAL(copy.flags.data(), original.flags.data());
}

BOOST_AUTO_TEST_SUITE_END()

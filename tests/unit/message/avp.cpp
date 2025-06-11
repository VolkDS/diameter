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
    AVP a{
        Code{1000},
        Flags{Flag::VendorSpecific} | Flags{Flag::Mandatory},
        10415,
        Unsigned32(uint32_t{0})
    };

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

BOOST_AUTO_TEST_SUITE_END()
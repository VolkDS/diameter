#include <boost/test/unit_test.hpp>

#include <diameter/message/avp/flags.h>
#include <diameter/message/header/command_flags.h>

namespace header = diameter::message::header;
namespace avp = diameter::message::avp;

BOOST_AUTO_TEST_SUITE(message_flags_tests)

BOOST_AUTO_TEST_CASE(command_flag_wire_positions)
{
    BOOST_CHECK_EQUAL(header::CommandFlags(header::CommandFlag::Request).data(), 0x80);
    BOOST_CHECK_EQUAL(header::CommandFlags(header::CommandFlag::Proxiable).data(), 0x40);
    BOOST_CHECK_EQUAL(header::CommandFlags(header::CommandFlag::Error).data(), 0x20);
    BOOST_CHECK_EQUAL(header::CommandFlags(header::CommandFlag::Retransmitted).data(), 0x10);
}

BOOST_AUTO_TEST_CASE(avp_flag_wire_positions)
{
    BOOST_CHECK_EQUAL(avp::Flags(avp::Flag::VendorSpecific).data(), 0x80);
    BOOST_CHECK_EQUAL(avp::Flags(avp::Flag::Mandatory).data(), 0x40);
    BOOST_CHECK_EQUAL(avp::Flags(avp::Flag::Protected).data(), 0x20);
}

BOOST_AUTO_TEST_CASE(empty_flags_observers)
{
    const header::CommandFlags flags{};
    BOOST_CHECK(flags.none());
    BOOST_CHECK(!flags.any());
    BOOST_CHECK(!flags.all());
    BOOST_CHECK_EQUAL(flags.count(), 0);
    BOOST_CHECK_EQUAL(flags.size(), 1);
    BOOST_CHECK_EQUAL(flags.to_ullong(), 0);
}

BOOST_AUTO_TEST_CASE(set_and_reset_preserve_other_bits)
{
    header::CommandFlags flags{0x0f};
    BOOST_CHECK(&flags.set(header::CommandFlag::Request) == &flags);
    flags.set(header::CommandFlag::Proxiable).set(header::CommandFlag::Error);
    BOOST_CHECK_EQUAL(flags.data(), 0xef);
    BOOST_CHECK_EQUAL(flags.count(), 7);
    BOOST_CHECK(&flags.reset(header::CommandFlag::Error) == &flags);
    BOOST_CHECK_EQUAL(flags.data(), 0xcf);
    flags.set(header::CommandFlag::Request, false);
    BOOST_CHECK_EQUAL(flags.data(), 0x4f);
    BOOST_CHECK(&flags.reset() == &flags);
    BOOST_CHECK(flags.none());
}

BOOST_AUTO_TEST_CASE(bitwise_operations_do_not_mutate_operands)
{
    const avp::Flags left{0xa5};
    const avp::Flags right{0x63};
    BOOST_CHECK_EQUAL((left | right).data(), 0xe7);
    BOOST_CHECK_EQUAL((left & right).data(), 0x21);
    BOOST_CHECK_EQUAL((left ^ right).data(), 0xc6);
    BOOST_CHECK_EQUAL(left.data(), 0xa5);
    BOOST_CHECK_EQUAL(right.data(), 0x63);
}

BOOST_AUTO_TEST_CASE(raw_reserved_bits_are_retained)
{
    const header::CommandFlags flags{0xff};
    BOOST_CHECK(flags.all());
    BOOST_CHECK_EQUAL(flags.count(), 8);
    BOOST_CHECK_EQUAL(flags.data(), 0xff);
    BOOST_CHECK(flags[header::CommandFlag::Request]);
    BOOST_CHECK(flags[header::CommandFlag::Retransmitted]);
}

BOOST_AUTO_TEST_SUITE_END()

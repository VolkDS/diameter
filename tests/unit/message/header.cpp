#include <boost/test/unit_test.hpp>

#include <diameter/message/header/header.h>

using namespace diameter::message::header;

BOOST_AUTO_TEST_SUITE(message_header_tests)

BOOST_AUTO_TEST_CASE(default_constructor)
{
    Header h{};
    
    BOOST_CHECK_EQUAL(h.version, ProtocolVersion{});
    BOOST_CHECK_EQUAL(h.length, MessageLength{});
    BOOST_CHECK_EQUAL(h.command_flags[CommandFlag::Request], false);
    BOOST_CHECK_EQUAL(h.command_flags[CommandFlag::Proxiable], false);
    BOOST_CHECK_EQUAL(h.command_flags[CommandFlag::Error], false);
    BOOST_CHECK_EQUAL(h.command_flags[CommandFlag::Retransmitted], false);
    BOOST_CHECK_EQUAL(h.command_code, CommandCode{});
    BOOST_CHECK_EQUAL(h.application_id, ApplicationId{});
    BOOST_CHECK_EQUAL(h.hop_by_hop, HopByHopIdentifier{});
    BOOST_CHECK_EQUAL(h.end_to_end, EndToEndIdentifier{});
}

BOOST_AUTO_TEST_CASE(field_initialization)
{
    Header h {
        ProtocolVersion{1},
        MessageLength{300},
        CommandFlags{0x80},
        CommandCode{272},
        ApplicationId{16777251},
        HopByHopIdentifier{0xdeadbeef},
        EndToEndIdentifier{0xcafebabe}
    };

    BOOST_CHECK_EQUAL(h.version, ProtocolVersion{1});
    BOOST_CHECK_EQUAL(h.length, MessageLength{300});
    BOOST_CHECK_EQUAL(h.command_flags[CommandFlag::Request], true);
    BOOST_CHECK_EQUAL(h.command_flags[CommandFlag::Proxiable], false);
    BOOST_CHECK_EQUAL(h.command_flags[CommandFlag::Error], false);
    BOOST_CHECK_EQUAL(h.command_flags[CommandFlag::Retransmitted], false);
    BOOST_CHECK_EQUAL(h.command_code, CommandCode{272});
    BOOST_CHECK_EQUAL(h.application_id, ApplicationId{16777251});
    BOOST_CHECK_EQUAL(h.hop_by_hop, HopByHopIdentifier{0xdeadbeef});
    BOOST_CHECK_EQUAL(h.end_to_end, EndToEndIdentifier{0xcafebabe});
}

BOOST_AUTO_TEST_CASE(size_calculation)
{
    Header h{};

    BOOST_CHECK_EQUAL(h.size(), 20);
}

BOOST_AUTO_TEST_SUITE_END()
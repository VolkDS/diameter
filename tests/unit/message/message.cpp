#include <boost/test/unit_test.hpp>

#include <diameter/message/message.h>

#include <list>

using namespace diameter::message;

BOOST_AUTO_TEST_SUITE(message_tests)

BOOST_AUTO_TEST_CASE(default_constructor)
{
    Message m{};

    BOOST_CHECK_EQUAL(m.header.version, header::ProtocolVersion{});
    BOOST_CHECK_EQUAL(m.header.length, header::MessageLength{});
    BOOST_CHECK_EQUAL(m.header.command_flags[header::CommandFlag::Request], false);
    BOOST_CHECK_EQUAL(m.header.command_flags[header::CommandFlag::Proxiable], false);
    BOOST_CHECK_EQUAL(m.header.command_flags[header::CommandFlag::Error], false);
    BOOST_CHECK_EQUAL(m.header.command_flags[header::CommandFlag::Retransmitted], false);
    BOOST_CHECK_EQUAL(m.header.command_code, header::CommandCode{});
    BOOST_CHECK_EQUAL(m.header.application_id, header::ApplicationId{});
    BOOST_CHECK_EQUAL(m.header.hop_by_hop, header::HopByHopIdentifier{});
    BOOST_CHECK_EQUAL(m.header.end_to_end, header::EndToEndIdentifier{});
    BOOST_CHECK_EQUAL(m.avps.size(), 0);
}

BOOST_AUTO_TEST_CASE(field_initialization)
{
    Message m {
        {
            header::ProtocolVersion{1},
            header::MessageLength{0},
            header::CommandFlags{0x80},
            header::CommandCode{272},
            header::ApplicationId{16777251},
            header::HopByHopIdentifier{0xdeadbeef},
            header::EndToEndIdentifier{0xcafebabe}
        },
        std::list<avp::AVP>{
            avp::AVP{1000, avp::Flags{avp::Flag::VendorSpecific} | avp::Flags{avp::Flag::Mandatory}, 10415, avp::Integer32(1)},
            avp::AVP{1001, avp::Flags{avp::Flag::Mandatory}, std::nullopt, avp::Integer64(1)}
        }
    };

    BOOST_CHECK_EQUAL(m.header.version, header::ProtocolVersion{1});
    BOOST_CHECK_EQUAL(m.header.length, header::MessageLength{0});
    BOOST_CHECK_EQUAL(m.header.command_flags[header::CommandFlag::Request], true);
    BOOST_CHECK_EQUAL(m.header.command_flags[header::CommandFlag::Proxiable], false);
    BOOST_CHECK_EQUAL(m.header.command_flags[header::CommandFlag::Error], false);
    BOOST_CHECK_EQUAL(m.header.command_flags[header::CommandFlag::Retransmitted], false);
    BOOST_CHECK_EQUAL(m.header.command_code, header::CommandCode{272});
    BOOST_CHECK_EQUAL(m.header.application_id, header::ApplicationId{16777251});
    BOOST_CHECK_EQUAL(m.header.hop_by_hop, header::HopByHopIdentifier{0xdeadbeef});
    BOOST_CHECK_EQUAL(m.header.end_to_end, header::EndToEndIdentifier{0xcafebabe});
    BOOST_CHECK_EQUAL(m.avps.size(), 2);
}

BOOST_AUTO_TEST_CASE(move_constructor)
{
    Message m1 {
        {
            header::ProtocolVersion{1},
            header::MessageLength{0},
            header::CommandFlags{0x80},
            header::CommandCode{272},
            header::ApplicationId{16777251},
            header::HopByHopIdentifier{0xdeadbeef},
            header::EndToEndIdentifier{0xcafebabe}
        },
        std::list<avp::AVP>{
            avp::AVP{1000, avp::Flags{avp::Flag::VendorSpecific} | avp::Flags{avp::Flag::Mandatory}, 10415, avp::Integer32(1)},
            avp::AVP{1001, avp::Flags{avp::Flag::Mandatory}, std::nullopt, avp::Integer64(1)}
        }
    };

    Message m2 {std::move(m1)};
    BOOST_CHECK_EQUAL(m2.header.version, header::ProtocolVersion{1});
    BOOST_CHECK_EQUAL(m2.header.length, header::MessageLength{0});
    BOOST_CHECK_EQUAL(m2.header.command_flags[header::CommandFlag::Request], true);
    BOOST_CHECK_EQUAL(m2.header.command_flags[header::CommandFlag::Proxiable], false);
    BOOST_CHECK_EQUAL(m2.header.command_flags[header::CommandFlag::Error], false);
    BOOST_CHECK_EQUAL(m2.header.command_flags[header::CommandFlag::Retransmitted], false);
    BOOST_CHECK_EQUAL(m2.header.command_code, header::CommandCode{272});
    BOOST_CHECK_EQUAL(m2.header.application_id, header::ApplicationId{16777251});
    BOOST_CHECK_EQUAL(m2.header.hop_by_hop, header::HopByHopIdentifier{0xdeadbeef});
    BOOST_CHECK_EQUAL(m2.header.end_to_end, header::EndToEndIdentifier{0xcafebabe});
    BOOST_CHECK_EQUAL(m2.avps.size(), 2);
}

BOOST_AUTO_TEST_CASE(empty_size_calculation)
{
    Message m{};

    BOOST_CHECK_EQUAL(m.size(), 20);
}

BOOST_AUTO_TEST_CASE(full_size_calculation)
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

    BOOST_CHECK_EQUAL(m.size(), 192);
}

BOOST_AUTO_TEST_SUITE_END()
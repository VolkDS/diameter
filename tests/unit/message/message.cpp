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
    // clang-format off
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
    // clang-format on

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
    // clang-format off
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
    // clang-format on

    Message m2{std::move(m1)};
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
    // clang-format off
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
    // clang-format on

    BOOST_CHECK_EQUAL(m.size(), 192);
}

BOOST_AUTO_TEST_CASE(size_updates_header_length_and_is_idempotent)
{
    Message message{};
    message.header.length = 999;
    message.avps.push_back(avp::AVP{1, avp::Flags{}, std::nullopt, avp::UTF8String{"a"}});
    message.avps.push_back(avp::AVP{2, avp::Flags{avp::Flag::VendorSpecific}, 10415,
        avp::UTF8String{"abcde"}});
    BOOST_CHECK_EQUAL(message.size(), 52);
    BOOST_CHECK_EQUAL(message.header.length, 52);
    BOOST_CHECK_EQUAL(message.size(), 52);

    message.avps.pop_front();
    BOOST_CHECK_EQUAL(message.size(), 40);
    BOOST_CHECK_EQUAL(message.header.length, 40);
    message.avps.clear();
    BOOST_CHECK_EQUAL(message.size(), 20);
    BOOST_CHECK_EQUAL(message.header.length, 20);
}

BOOST_AUTO_TEST_CASE(size_includes_nested_grouped_avps)
{
    Message message{};
    const avp::AVP child{1, avp::Flags{}, std::nullopt, avp::UTF8String{"x"}};
    message.avps.push_back(avp::AVP{2, avp::Flags{}, std::nullopt,
        avp::Grouped{std::list<avp::AVP>{child, child}}});
    BOOST_CHECK_EQUAL(message.size(), 52);
    BOOST_CHECK_EQUAL(message.header.length, 52);
}

BOOST_AUTO_TEST_CASE(copy_constructor_preserves_order_and_owns_avps)
{
    Message original{};
    original.header.version = 1;
    original.header.hop_by_hop = 0x12345678;
    original.header.end_to_end = 0xabcdef01;
    original.avps.push_back(avp::AVP{1, avp::Flags{}, std::nullopt, avp::UTF8String{"first"}});
    original.avps.push_back(avp::AVP{1, avp::Flags{}, std::nullopt, avp::UTF8String{"second"}});
    const auto size = original.size();

    Message copy{original};
    BOOST_CHECK_EQUAL(copy.header.length, size);
    BOOST_CHECK_EQUAL(copy.header.version, 1);
    BOOST_CHECK_EQUAL(copy.header.hop_by_hop, 0x12345678);
    BOOST_CHECK_EQUAL(copy.header.end_to_end, 0xabcdef01);
    BOOST_REQUIRE_EQUAL(copy.avps.size(), 2);
    BOOST_CHECK_EQUAL(*std::get<avp::UTF8String>(copy.avps.front().value), "first");
    BOOST_CHECK_EQUAL(*std::get<avp::UTF8String>(copy.avps.back().value), "second");

    *std::get<avp::UTF8String>(copy.avps.front().value) = "changed";
    copy.avps.pop_back();
    BOOST_CHECK_EQUAL(original.avps.size(), 2);
    BOOST_CHECK_EQUAL(*std::get<avp::UTF8String>(original.avps.front().value), "first");
}

BOOST_AUTO_TEST_CASE(copy_assignment_replaces_existing_header_and_avps)
{
    Message original{};
    original.header.command_code = 280;
    original.header.command_flags.set(header::CommandFlag::Request);
    original.avps.push_back(avp::AVP{10, avp::Flags{}, std::nullopt, avp::Unsigned32{123u}});
    original.size();

    Message destination{};
    destination.header.command_code = 257;
    destination.avps.push_back(avp::AVP{99, avp::Flags{}, std::nullopt, avp::UTF8String{"old"}});
    destination = original;
    BOOST_CHECK_EQUAL(destination.header.command_code, 280);
    BOOST_CHECK(destination.header.command_flags[header::CommandFlag::Request]);
    BOOST_CHECK_EQUAL(destination.header.length, 32);
    BOOST_REQUIRE_EQUAL(destination.avps.size(), 1);
    BOOST_CHECK_EQUAL(destination.avps.front().code, 10);
    BOOST_CHECK_EQUAL(*std::get<avp::Unsigned32>(destination.avps.front().value), 123);
}

BOOST_AUTO_TEST_CASE(move_assignment_transfers_header_and_grouped_payload)
{
    Message original{};
    original.header.command_code = 272;
    original.header.application_id = 4;
    original.header.hop_by_hop = 0xffffffff;
    original.header.end_to_end = 0x87654321;
    const avp::AVP child{1, avp::Flags{}, std::nullopt, avp::UTF8String{"retained"}};
    original.avps.push_back(avp::AVP{2, avp::Flags{avp::Flag::VendorSpecific}, 10415,
        avp::Grouped{std::list<avp::AVP>{child}}});
    const auto expected_size = original.size();

    Message destination{};
    destination.avps.push_back(child);
    destination = std::move(original);
    BOOST_CHECK_EQUAL(destination.header.command_code, 272);
    BOOST_CHECK_EQUAL(destination.header.application_id, 4);
    BOOST_CHECK_EQUAL(destination.header.hop_by_hop, 0xffffffff);
    BOOST_CHECK_EQUAL(destination.header.end_to_end, 0x87654321);
    BOOST_CHECK_EQUAL(destination.header.length, expected_size);
    BOOST_REQUIRE_EQUAL(destination.avps.size(), 1);
    BOOST_CHECK_EQUAL(destination.avps.front().vendor_id.value(), 10415);
    const auto& children = *std::get<avp::Grouped>(destination.avps.front().value);
    BOOST_REQUIRE_EQUAL(children.size(), 1);
    BOOST_CHECK_EQUAL(*std::get<avp::UTF8String>(children.front().value), "retained");
    BOOST_CHECK_EQUAL(destination.size(), expected_size);
}

BOOST_AUTO_TEST_SUITE_END()

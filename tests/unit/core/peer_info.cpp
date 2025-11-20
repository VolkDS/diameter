#include <boost/test/unit_test.hpp>

#include <diameter/core/peer/info.h>
#include <diameter/message/avp/vendor_id.h>

#include <list>

using namespace diameter::core::peer;

namespace dm = diameter::message;
namespace dmh = diameter::message::header;
namespace dma = diameter::message::avp;

BOOST_AUTO_TEST_SUITE(peer_info_tests)

BOOST_AUTO_TEST_CASE(default_constructor)
{
    PeerInfo peer_info {};

    BOOST_CHECK_EQUAL(peer_info.origin_host, std::string());
    BOOST_CHECK_EQUAL(peer_info.origin_realm, std::string());
    BOOST_CHECK_EQUAL(peer_info.vendor_id, diameter::message::avp::VendorId{});
    BOOST_CHECK_EQUAL(peer_info.product_name, std::string());

    BOOST_CHECK_EQUAL(peer_info.inband_security_supported, false);
    BOOST_CHECK(peer_info.supported_vendor_ids.empty());
    BOOST_CHECK(peer_info.auth_application_ids.empty());
    BOOST_CHECK(peer_info.acct_application_ids.empty());
    BOOST_CHECK(peer_info.vendor_specific_application_ids.empty());
}

BOOST_AUTO_TEST_CASE(construct_from_message)
{
    dm::Message m {
        {
            dmh::ProtocolVersion{dmh::ProtocolVersionV::V01},
            dmh::MessageLength{0},
            dmh::CommandFlags{0x80},
            dmh::CommandCode{257},
            dmh::ApplicationId{0},
            dmh::HopByHopIdentifier{0x6ad1d314},
            dmh::EndToEndIdentifier{0x77287404}
        },
        std::list<dma::AVP>{
            dma::AVP{264, dma::Flags{dma::Flag::Mandatory}, std::nullopt, dma::DiameterIdentity("testhost.epc.mnc000.mcc000.3gppnetwork.org")},
            dma::AVP{296, dma::Flags{dma::Flag::Mandatory}, std::nullopt, dma::DiameterIdentity("epc.mnc000.mcc000.3gppnetwork.org")},
            dma::AVP{257, dma::Flags{dma::Flag::Mandatory}, std::nullopt, dma::Address("127.0.0.1")},
            dma::AVP{266, dma::Flags{dma::Flag::Mandatory}, std::nullopt, dma::Unsigned32(uint32_t{10415})},
            dma::AVP{269, dma::Flags{}, std::nullopt, dma::UTF8String("ExampleProduct")},
            dma::AVP{258, dma::Flags{dma::Flag::Mandatory}, std::nullopt, dma::Unsigned32(uint32_t{4})},
            dma::AVP{265, dma::Flags{dma::Flag::Mandatory}, std::nullopt, dma::Unsigned32(uint32_t{10415})}
        }
    };
}

BOOST_AUTO_TEST_SUITE_END()
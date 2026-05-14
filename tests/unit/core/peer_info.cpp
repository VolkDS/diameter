#include <boost/test/unit_test.hpp>

#include <diameter/core/peer/info.h>
#include <diameter/message/avp/vendor_id.h>
#include <diameter/application/common/common.h>

#include <list>

using namespace diameter::core::peer;

namespace da = diameter::application;
namespace dm = diameter::message;
namespace dmh = diameter::message::header;
namespace dma = diameter::message::avp;

BOOST_AUTO_TEST_SUITE(peer_info_tests)

BOOST_AUTO_TEST_CASE(default_constructor)
{
    PeerInfo peer_info {};

    BOOST_CHECK_EQUAL(peer_info.origin_host, std::string());
    BOOST_CHECK_EQUAL(peer_info.origin_realm, std::string());
    BOOST_CHECK_EQUAL(peer_info.vendor_id, diameter::message::avp::VendorId {});
    BOOST_CHECK_EQUAL(peer_info.product_name, std::string());

    BOOST_CHECK_EQUAL(peer_info.inband_security_supported, false);
    BOOST_CHECK(peer_info.supported_vendor_ids.empty());
    BOOST_CHECK(peer_info.auth_application_ids.empty());
    BOOST_CHECK(peer_info.acct_application_ids.empty());
    BOOST_CHECK(peer_info.vendor_specific_application_ids.empty());
}

BOOST_AUTO_TEST_CASE(construct_from_message)
{
    // <CER> ::= < Diameter Header: 257, REQ >
    //           { Origin-Host }
    //           { Origin-Realm }
    //        1* { Host-IP-Address }
    //           { Vendor-Id }
    //           { Product-Name }
    //           [ Origin-State-Id ]
    //         * [ Supported-Vendor-Id ]
    //         * [ Auth-Application-Id ]
    //         * [ Inband-Security-Id ]
    //         * [ Acct-Application-Id ]
    //         * [ Vendor-Specific-Application-Id ]
    //           [ Firmware-Revision ]
    //         * [ AVP ]
    dm::Message m {
        {
            dmh::ProtocolVersion{dmh::ProtocolVersionV::V01},
            dmh::MessageLength{0},
            dmh::CommandFlags{dmh::CommandFlag::Request},
            dmh::CommandCode{da::common::CommandV::CapabilitiesExchange},
            dmh::ApplicationId{dmh::ApplicationV::Common},
            dmh::HopByHopIdentifier{1},
            dmh::EndToEndIdentifier{1}
        },
        std::list<dma::AVP>{
            dma::AVP{da::common::AvpCodeV::OriginHost, dma::Flags{dma::Flag::Mandatory}, std::nullopt, dma::DiameterIdentity("testhost.epc.mnc000.mcc000.3gppnetwork.org")},
            dma::AVP{da::common::AvpCodeV::OriginRealm, dma::Flags{dma::Flag::Mandatory}, std::nullopt, dma::DiameterIdentity("epc.mnc000.mcc000.3gppnetwork.org")},
            dma::AVP{da::common::AvpCodeV::HostIPAddress, dma::Flags{dma::Flag::Mandatory}, std::nullopt, dma::Address("127.0.0.1")},
            dma::AVP{da::common::AvpCodeV::HostIPAddress, dma::Flags{dma::Flag::Mandatory}, std::nullopt, dma::Address("127.0.0.2")},
            dma::AVP{da::common::AvpCodeV::VendorId, dma::Flags{dma::Flag::Mandatory}, std::nullopt, dma::Unsigned32(uint32_t{1})},
            dma::AVP{da::common::AvpCodeV::ProductName, dma::Flags{}, std::nullopt, dma::UTF8String("ExampleProduct")},
            dma::AVP{da::common::AvpCodeV::SupportedVendorId, dma::Flags{}, std::nullopt, dma::Unsigned32(uint32_t{2})},
            dma::AVP{da::common::AvpCodeV::SupportedVendorId, dma::Flags{}, std::nullopt, dma::Unsigned32(uint32_t{3})},
            dma::AVP{da::common::AvpCodeV::SupportedVendorId, dma::Flags{}, std::nullopt, dma::Unsigned32(uint32_t{4})},
            dma::AVP{da::common::AvpCodeV::AuthApplicationId, dma::Flags{}, std::nullopt, dma::Unsigned32(uint32_t{5})},
            dma::AVP{da::common::AvpCodeV::AuthApplicationId, dma::Flags{}, std::nullopt, dma::Unsigned32(uint32_t{6})},
            dma::AVP{da::common::AvpCodeV::AcctApplicationId, dma::Flags{}, std::nullopt, dma::Unsigned32(uint32_t{7})},
            dma::AVP{da::common::AvpCodeV::AcctApplicationId, dma::Flags{}, std::nullopt, dma::Unsigned32(uint32_t{8})},
            dma::AVP{da::common::AvpCodeV::VendorSpecificApplicationId, dma::Flags{}, std::nullopt, dma::Grouped(dma::Grouped::value_type{
                dma::AVP{da::common::AvpCodeV::VendorId, dma::Flags{dma::Flag::Mandatory}, std::nullopt, dma::Unsigned32(uint32_t{9})},
                dma::AVP{da::common::AvpCodeV::AuthApplicationId, dma::Flags{}, std::nullopt, dma::Unsigned32(uint32_t{10})},
            })},
            dma::AVP{da::common::AvpCodeV::VendorSpecificApplicationId, dma::Flags{}, std::nullopt, dma::Grouped(dma::Grouped::value_type{
                dma::AVP{da::common::AvpCodeV::VendorId, dma::Flags{dma::Flag::Mandatory}, std::nullopt, dma::Unsigned32(uint32_t{11})},
                dma::AVP{da::common::AvpCodeV::AcctApplicationId, dma::Flags{}, std::nullopt, dma::Unsigned32(uint32_t{12})},
            })}
        }
    };

    auto m_ptr = std::make_shared<dm::Message>(std::move(m));
    PeerInfo peer_info = make_peer_info(m_ptr);

    BOOST_CHECK_EQUAL(peer_info.origin_host, "testhost.epc.mnc000.mcc000.3gppnetwork.org");
    BOOST_CHECK_EQUAL(peer_info.origin_realm, "epc.mnc000.mcc000.3gppnetwork.org");
    BOOST_CHECK_EQUAL(peer_info.vendor_id, 1);
    BOOST_CHECK_EQUAL(peer_info.product_name, "ExampleProduct");

    {
        auto expected_data = std::list<uint32_t>{2, 3, 4};
        BOOST_CHECK_EQUAL_COLLECTIONS(
            peer_info.supported_vendor_ids.begin(),
            peer_info.supported_vendor_ids.end(),
            expected_data.begin(),
            expected_data.end()
        );
    }
    {
        auto expected_data = std::list<uint32_t>{5, 6};
        BOOST_CHECK_EQUAL_COLLECTIONS(
            peer_info.auth_application_ids.begin(),
            peer_info.auth_application_ids.end(),
            expected_data.begin(),
            expected_data.end()
        );
    }
    {
        auto expected_data = std::list<uint32_t>{7, 8};
        BOOST_CHECK_EQUAL_COLLECTIONS(
            peer_info.acct_application_ids.begin(),
            peer_info.acct_application_ids.end(),
            expected_data.begin(),
            expected_data.end()
        );
    }
}

BOOST_AUTO_TEST_SUITE_END()

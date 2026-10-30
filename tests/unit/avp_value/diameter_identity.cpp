#include <boost/test/unit_test.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <diameter/message/avp/avp.h>

using namespace diameter;

BOOST_AUTO_TEST_SUITE(avp_diameter_identity)

BOOST_AUTO_TEST_CASE(constructor)
{
    auto value1 = message::avp::DiameterIdentity("test");
    auto value2 = message::avp::DiameterIdentity("911.gov");
    auto value3 = message::avp::DiameterIdentity("911");
    auto value4 = message::avp::DiameterIdentity("a-.com");
    auto value5 = message::avp::DiameterIdentity("-a.com");
    auto value6 = message::avp::DiameterIdentity("a.com");
    auto value7 = message::avp::DiameterIdentity("a.66");
    auto value8 = message::avp::DiameterIdentity("my_host.com");
    auto value9 = message::avp::DiameterIdentity("typical-hostname33.whatever.co.uk");

    auto value10 = message::avp::DiameterIdentity("test-link.epc.mnc000.mcc000.3gppnetwork.org");
    auto value11 = message::avp::DiameterIdentity("link.epc.mnc099.mcc000.3gppnetwork.org");
    auto value12 = message::avp::DiameterIdentity("dra.test.epc.mnc099.mcc000.3gppnetwork.org");
    auto value13 = message::avp::DiameterIdentity("mos.epc.mnc099.mcc000.3gppnetwork.org");

    BOOST_CHECK_EQUAL(value1->validate(), false);
    BOOST_CHECK_EQUAL(value2->validate(), true);
    BOOST_CHECK_EQUAL(value3->validate(), false);
    BOOST_CHECK_EQUAL(value4->validate(), false);
    BOOST_CHECK_EQUAL(value5->validate(), false);
    BOOST_CHECK_EQUAL(value6->validate(), true);
    BOOST_CHECK_EQUAL(value7->validate(), false);
    BOOST_CHECK_EQUAL(value8->validate(), false);
    BOOST_CHECK_EQUAL(value9->validate(), true);

    BOOST_CHECK_EQUAL(value10->validate(), true);
    BOOST_CHECK_EQUAL(value11->validate(), true);
    BOOST_CHECK_EQUAL(value12->validate(), true);
    BOOST_CHECK_EQUAL(value13->validate(), true);
}

BOOST_AUTO_TEST_CASE(value_and_size_preserve_ascii_case)
{
    const std::string text{"Peer.Example.COM"};
    const auto identity = message::avp::DiameterIdentity(text);
    BOOST_CHECK(identity->validate());
    BOOST_CHECK_EQUAL(identity->value(), text);
    BOOST_CHECK_EQUAL(identity.size(), text.size());
}

BOOST_AUTO_TEST_CASE(dns_label_length_boundaries)
{
    const auto maximum = message::avp::DiameterIdentity(std::string(63, 'a') + ".example.com");
    const auto excessive = message::avp::DiameterIdentity(std::string(64, 'a') + ".example.com");
    BOOST_CHECK(maximum->validate());
    BOOST_CHECK(!excessive->validate());
}

BOOST_AUTO_TEST_CASE(dns_name_length_boundaries)
{
    const std::string maximum = std::string(63, 'a') + '.' + std::string(63, 'b') + '.'
        + std::string(63, 'c') + '.' + std::string(61, 'd');
    BOOST_REQUIRE_EQUAL(maximum.size(), 253);
    BOOST_CHECK(message::avp::DiameterIdentity(maximum)->validate());
    BOOST_CHECK(!message::avp::DiameterIdentity(maximum + 'd')->validate());
}

BOOST_AUTO_TEST_CASE(invalid_dns_syntax_is_rejected)
{
    const std::vector<std::string> texts{"", ".example.com", "peer..example.com",
        "peer example.com", "peer.example.com:3868", "peer/example.com",
        std::string{"peer\0.example.com", 17}, "peer.\xc3\xa9xample.com"};
    for (const auto& text : texts) {
        BOOST_TEST_CONTEXT("identity size " << text.size()) {
            const auto identity = message::avp::DiameterIdentity(text);
            BOOST_CHECK(!identity->validate());
        }
    }
}

BOOST_AUTO_TEST_CASE(internationalized_names_accept_ascii_a_labels)
{
    // RFC 6733 Appendix D requires the ASCII-compatible form of internationalized names.
    const auto identity = message::avp::DiameterIdentity("xn--e1afmkfd.xn--p1ai");
    BOOST_CHECK(identity->validate());
}

BOOST_AUTO_TEST_SUITE_END()

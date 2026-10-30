#include <boost/test/unit_test.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <diameter/message/avp/avp.h>

using namespace diameter;

BOOST_AUTO_TEST_SUITE(avp_ip_filter_rule)

BOOST_AUTO_TEST_CASE(constructor)
{
    auto value1 = message::avp::IPFilterRule("deny out udp from any to any 5060");
    auto value2 = message::avp::IPFilterRule("permit in tcp from any to any 443");
    auto value3 = message::avp::IPFilterRule("deny in icmp from 192.168.2.5/32 to any");

    auto value4 = message::avp::IPFilterRule("permit in ip from any to any");
    auto value5 = message::avp::IPFilterRule("deny out tcp from 10.0.0.0/8 to 192.168.1.1/32");

    auto value6
        = message::avp::IPFilterRule("permit in udp from 192.168.1.0/24 to any 5060"); // SIP traffic
    auto value7
        = message::avp::IPFilterRule("deny in icmp from any to 10.0.0.1/32"); // Block ping
    auto value8 = message::avp::IPFilterRule("permit out tcp from any to any 443"); // HTTPS

    auto value9 = message::avp::IPFilterRule(
        "permit in tcp from any to any 80-82"); // HTTP and alternative ports
    auto value10 = message::avp::IPFilterRule("deny out udp from 10.0.0.5/32 to any 123"); // NTP

    auto value11 = message::avp::IPFilterRule("permit in ip from 2001:db8::/32 to any");
    auto value12
        = message::avp::IPFilterRule("deny out tcp from ::1/128 to any 22"); // SSH over IPv6

    auto value13 = message::avp::IPFilterRule(
        "permit in ip from 192.168.1.100/32 to 192.168.1.200/32"); // Host-specific rule
    auto value14
        = message::avp::IPFilterRule("deny in ip from any to 224.0.0.0/4"); // Block multicast

    auto value15
        = message::avp::IPFilterRule("permit out ip from any to any"); // All outgoing traffic
    auto value16 = message::avp::IPFilterRule(
        "deny in ip from 10.0.0.0/8 to any"); // Block the internal network

    auto value17 = message::avp::IPFilterRule(
        "permit in ip from 0.0.0.0/0 to 255.255.255.255/32"); // All IPv4 traffic
    auto value18 = message::avp::IPFilterRule("deny out ip from ::/0 to any"); // All IPv6 traffic

    auto value19 = message::avp::IPFilterRule(
        "allow in ip from any to any"); // Invalid action
    auto value21
        = message::avp::IPFilterRule("permit in ip from any to any 100000"); // Invalid port

    BOOST_CHECK_EQUAL(value1->validate(), true);
    BOOST_CHECK_EQUAL(value2->validate(), true);
    BOOST_CHECK_EQUAL(value3->validate(), true);
    BOOST_CHECK_EQUAL(value4->validate(), true);
    BOOST_CHECK_EQUAL(value5->validate(), true);
    BOOST_CHECK_EQUAL(value6->validate(), true);
    BOOST_CHECK_EQUAL(value7->validate(), true);
    BOOST_CHECK_EQUAL(value8->validate(), true);
    BOOST_CHECK_EQUAL(value9->validate(), true);
    BOOST_CHECK_EQUAL(value10->validate(), true);
    BOOST_CHECK_EQUAL(value11->validate(), true);
    BOOST_CHECK_EQUAL(value12->validate(), true);
    BOOST_CHECK_EQUAL(value13->validate(), true);
    BOOST_CHECK_EQUAL(value14->validate(), true);
    BOOST_CHECK_EQUAL(value15->validate(), true);
    BOOST_CHECK_EQUAL(value16->validate(), true);
    BOOST_CHECK_EQUAL(value17->validate(), true);
    BOOST_CHECK_EQUAL(value18->validate(), true);
    BOOST_CHECK_EQUAL(value19->validate(), false);
    BOOST_CHECK_EQUAL(value21->validate(), false);
}

BOOST_AUTO_TEST_CASE(validated_fields_preserve_rule_content)
{
    const std::string text{"deny out tcp from 192.0.2.0/24 1024-2048 to 198.51.100.1/32 443"};
    auto rule = message::avp::IPFilterRule(text);
    BOOST_REQUIRE(rule->validate());
    BOOST_CHECK_EQUAL(rule->value(), text);
    BOOST_CHECK_EQUAL(rule.size(), text.size());
    BOOST_CHECK_EQUAL(rule->action(), "deny");
    BOOST_CHECK_EQUAL(rule->direction(), "out");
    BOOST_CHECK_EQUAL(rule->protocol(), "tcp");
    BOOST_CHECK_EQUAL(rule->src(), "192.0.2.0/24");
    BOOST_CHECK_EQUAL(rule->dst(), "198.51.100.1/32");
    BOOST_CHECK(rule->validate());
}

BOOST_AUTO_TEST_CASE(invalid_actions_and_incomplete_rules)
{
    const char* texts[]{"", "allow in ip from any to any", "permit sideways ip from any to any",
        "permit in ip any to any", "permit in ip from any", "permit in ip from any to",
        "permit in ip from any to any garbage"};
    for (const auto* text : texts) {
        BOOST_TEST_CONTEXT(text) {
            auto rule = message::avp::IPFilterRule(text);
            BOOST_CHECK(!rule->validate());
        }
    }
}

BOOST_AUTO_TEST_CASE(ipv4_octets_must_be_in_range)
{
    const char* texts[]{"permit in ip from 256.0.0.1 to any",
        "permit in ip from any to 192.0.2.999", "permit in ip from 999.999.999.999 to any"};
    for (const auto* text : texts) {
        BOOST_TEST_CONTEXT(text) {
            auto rule = message::avp::IPFilterRule(text);
            BOOST_CHECK(!rule->validate());
        }
    }
}

BOOST_AUTO_TEST_CASE(ipv6_address_syntax_must_be_valid)
{
    const char* texts[]{"permit in ip from 2001:::1 to any",
        "permit in ip from any to 12345::1", "permit in ip from deadbeef to any"};
    for (const auto* text : texts) {
        BOOST_TEST_CONTEXT(text) {
            auto rule = message::avp::IPFilterRule(text);
            BOOST_CHECK(!rule->validate());
        }
    }
}

BOOST_AUTO_TEST_CASE(prefix_length_must_fit_address_family)
{
    const char* texts[]{"permit in ip from 192.0.2.0/33 to any",
        "permit in ip from any to 198.51.100.0/99", "permit in ip from 2001:db8::/129 to any"};
    for (const auto* text : texts) {
        BOOST_TEST_CONTEXT(text) {
            auto rule = message::avp::IPFilterRule(text);
            BOOST_CHECK(!rule->validate());
        }
    }
}

BOOST_AUTO_TEST_CASE(port_numbers_must_fit_sixteen_bits)
{
    const char* texts[]{"permit in tcp from any 65536 to any",
        "permit in udp from any to any 99999", "permit in tcp from any to any 80-65536"};
    for (const auto* text : texts) {
        BOOST_TEST_CONTEXT(text) {
            auto rule = message::avp::IPFilterRule(text);
            BOOST_CHECK(!rule->validate());
        }
    }
}

BOOST_AUTO_TEST_CASE(numeric_ip_protocol_is_supported)
{
    // RFC 6733 defines proto as "ip" or an IP protocol number from 0 through 255.
    const char* texts[]{"permit in 6 from any to any 443",
        "permit in 17 from any to any 53", "deny out 1 from any to any",
        "permit in 0 from any to any", "permit in 255 from any to any"};
    for (const auto* text : texts) {
        BOOST_TEST_CONTEXT(text) {
            auto rule = message::avp::IPFilterRule(text);
            BOOST_CHECK(rule->validate());
        }
    }
}

BOOST_AUTO_TEST_CASE(assigned_and_negated_addresses_are_supported)
{
    const char* texts[]{"permit in ip from assigned to any",
        "deny out ip from any to !192.0.2.0/24"};
    for (const auto* text : texts) {
        BOOST_TEST_CONTEXT(text) {
            auto rule = message::avp::IPFilterRule(text);
            BOOST_CHECK(rule->validate());
        }
    }
}

BOOST_AUTO_TEST_CASE(comma_separated_port_lists_are_supported)
{
    auto rule = message::avp::IPFilterRule("permit in 6 from any to any 80,443,8000-8080");
    BOOST_CHECK(rule->validate());
}

BOOST_AUTO_TEST_CASE(tcp_established_option_is_supported)
{
    auto rule = message::avp::IPFilterRule("permit in 6 from any to any established");
    BOOST_CHECK(rule->validate());
}

BOOST_AUTO_TEST_SUITE_END()

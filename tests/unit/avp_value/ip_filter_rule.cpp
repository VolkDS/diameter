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

    auto value6 = message::avp::IPFilterRule("permit in udp from 192.168.1.0/24 to any 5060");  // SIP-трафик
    auto value7 = message::avp::IPFilterRule("deny in icmp from any to 10.0.0.1/32");           // Блокировка ping
    auto value8 = message::avp::IPFilterRule("permit out tcp from any to any 443");             // HTTPS

    auto value9 = message::avp::IPFilterRule("permit in tcp from any to any 80-82");            // HTTP + альтернативные порты
    auto value10 = message::avp::IPFilterRule("deny out udp from 10.0.0.5/32 to any 123");       // NTP

    auto value11 = message::avp::IPFilterRule("permit in ip from 2001:db8::/32 to any");
    auto value12 = message::avp::IPFilterRule("deny out tcp from ::1/128 to any 22");            // SSH для IPv6

    auto value13 = message::avp::IPFilterRule("permit in ip from 192.168.1.100/32 to 192.168.1.200/32");  // Точечное правило
    auto value14 = message::avp::IPFilterRule("deny in ip from any to 224.0.0.0/4");                      // Блокировка multicast

    auto value15 = message::avp::IPFilterRule("permit out ip from any to any");                  // Весь исходящий трафик
    auto value16 = message::avp::IPFilterRule("deny in ip from 10.0.0.0/8 to any");              // Блокировка всей внутренней сети

    auto value17 = message::avp::IPFilterRule("permit in ip from 0.0.0.0/0 to 255.255.255.255/32");  // Весь IPv4-трафик
    auto value18 = message::avp::IPFilterRule("deny out ip from ::/0 to any");                       // Весь IPv6-трафик

    auto value19 = message::avp::IPFilterRule("allow in ip from any to any");                     // Неверное действие (`allow` вместо `permit/deny`)
    //auto value20 = message::avp::IPFilterRule("permit in tcp from 256.256.256.256 to any");       // Невалидный IP-адрес
    auto value21 = message::avp::IPFilterRule("permit in ip from any to any 100000");             // Неверный порт

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
    //BOOST_CHECK_EQUAL(value20->validate(), false);
    BOOST_CHECK_EQUAL(value21->validate(), false);
}

BOOST_AUTO_TEST_SUITE_END()

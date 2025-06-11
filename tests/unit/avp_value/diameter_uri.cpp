#include <boost/test/unit_test.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <diameter/message/avp/avp.h>

using namespace diameter;

BOOST_AUTO_TEST_SUITE(avp_diameter_uri)

BOOST_AUTO_TEST_CASE(constructor)
{
    auto value1 = message::avp::DiameterURI("aaa://host.example.com;transport=tcp");
    auto value2 = message::avp::DiameterURI("aaa://host.example.com:6666;transport=tcp");
    auto value3 = message::avp::DiameterURI("aaa://host.example.com;protocol=diameter");
    auto value4 = message::avp::DiameterURI("aaa://host.example.com:6666;protocol=diameter");
    auto value5 = message::avp::DiameterURI("aaa://host.example.com:6666;transport=tcp;protocol=radius");
    auto value6 = message::avp::DiameterURI("aaa://host.example.com:1813;transport=udp;protocol=tacacs+");
    auto value7 = message::avp::DiameterURI("aaa://server.com");
    auto value8 = message::avp::DiameterURI("aaa://server.com:1234");
    auto value9 = message::avp::DiameterURI("aaas://server.com:1234;transport=tcp");

    auto value10 = message::avp::DiameterURI("http://server.com:1234;transport=tcp");
    auto value11 = message::avp::DiameterURI("https://server.com:1234;transport=udp");
    auto value12 = message::avp::DiameterURI("https://127.0.0.1:1234;transport=udp");
    auto value13 = message::avp::DiameterURI("aaa://host.example.com:1813;;transport=tcp;protocol=diameter");
    auto value14 = message::avp::DiameterURI("aaa://host.example.com:1813;transport=tcp;protocol=diameter;");

    BOOST_CHECK_EQUAL(value1->validate(), true);
    BOOST_CHECK_EQUAL(value2->validate(), true);
    BOOST_CHECK_EQUAL(value3->validate(), true);
    BOOST_CHECK_EQUAL(value4->validate(), true);
    BOOST_CHECK_EQUAL(value5->validate(), true);
    BOOST_CHECK_EQUAL(value6->validate(), true);
    BOOST_CHECK_EQUAL(value7->validate(), true);
    BOOST_CHECK_EQUAL(value8->validate(), true);
    BOOST_CHECK_EQUAL(value9->validate(), true);
    BOOST_CHECK_EQUAL(value10->validate(), false);
    BOOST_CHECK_EQUAL(value11->validate(), false);
    BOOST_CHECK_EQUAL(value12->validate(), false);
    BOOST_CHECK_EQUAL(value13->validate(), false);
    BOOST_CHECK_EQUAL(value14->validate(), false);

    BOOST_CHECK_EQUAL(value1->scheme(), "aaa");
    BOOST_CHECK_EQUAL(value2->scheme(), "aaa");
    BOOST_CHECK_EQUAL(value3->scheme(), "aaa");
    BOOST_CHECK_EQUAL(value4->scheme(), "aaa");
    BOOST_CHECK_EQUAL(value5->scheme(), "aaa");
    BOOST_CHECK_EQUAL(value6->scheme(), "aaa");
    BOOST_CHECK_EQUAL(value7->scheme(), "aaa");
    BOOST_CHECK_EQUAL(value8->scheme(), "aaa");
    BOOST_CHECK_EQUAL(value9->scheme(), "aaas");

    BOOST_CHECK_EQUAL(value1->fqdn(), "host.example.com");
    BOOST_CHECK_EQUAL(value2->fqdn(), "host.example.com");
    BOOST_CHECK_EQUAL(value3->fqdn(), "host.example.com");
    BOOST_CHECK_EQUAL(value4->fqdn(), "host.example.com");
    BOOST_CHECK_EQUAL(value5->fqdn(), "host.example.com");
    BOOST_CHECK_EQUAL(value6->fqdn(), "host.example.com");
    BOOST_CHECK_EQUAL(value7->fqdn(), "server.com");
    BOOST_CHECK_EQUAL(value8->fqdn(), "server.com");
    BOOST_CHECK_EQUAL(value9->fqdn(), "server.com");

    BOOST_CHECK_EQUAL(value1->port().has_value(), false);
    BOOST_CHECK_EQUAL(value2->port().value(), 6666);
    BOOST_CHECK_EQUAL(value3->port().has_value(), false);
    BOOST_CHECK_EQUAL(value4->port().value(), 6666);
    BOOST_CHECK_EQUAL(value5->port().value(), 6666);
    BOOST_CHECK_EQUAL(value6->port().value(), 1813);
    BOOST_CHECK_EQUAL(value7->port().has_value(), false);
    BOOST_CHECK_EQUAL(value8->port().value(), 1234);
    BOOST_CHECK_EQUAL(value9->port().value(), 1234);

    BOOST_CHECK_EQUAL(value1->transport().value(), "tcp");
    BOOST_CHECK_EQUAL(value2->transport().value(), "tcp");
    BOOST_CHECK_EQUAL(value3->transport().has_value(), false);
    BOOST_CHECK_EQUAL(value4->transport().has_value(), false);
    BOOST_CHECK_EQUAL(value5->transport().value(), "tcp");
    BOOST_CHECK_EQUAL(value6->transport().value(), "udp");
    BOOST_CHECK_EQUAL(value7->transport().has_value(), false);
    BOOST_CHECK_EQUAL(value8->transport().has_value(), false);
    BOOST_CHECK_EQUAL(value9->transport().value(), "tcp");

    BOOST_CHECK_EQUAL(value1->protocol().has_value(), false);
    BOOST_CHECK_EQUAL(value2->protocol().has_value(), false);
    BOOST_CHECK_EQUAL(value3->protocol().value(), "diameter");
    BOOST_CHECK_EQUAL(value4->protocol().value(), "diameter");
    BOOST_CHECK_EQUAL(value5->protocol().value(), "radius");
    BOOST_CHECK_EQUAL(value6->protocol().value(), "tacacs+");
    BOOST_CHECK_EQUAL(value7->protocol().has_value(), false);
    BOOST_CHECK_EQUAL(value8->protocol().has_value(), false);
    BOOST_CHECK_EQUAL(value9->protocol().has_value(), false);
}

BOOST_AUTO_TEST_SUITE_END()

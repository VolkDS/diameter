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

BOOST_AUTO_TEST_SUITE_END()

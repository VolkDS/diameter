#include <boost/test/unit_test.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <diameter/message/avp/avp.h>

#include "../../helpers/string_to_time.h"

using namespace diameter;

BOOST_AUTO_TEST_SUITE(avp_time)

BOOST_AUTO_TEST_CASE(constructor_from_unix)
{
    // RFC 5905 Examples
    auto time_ex1 = message::avp::Time(string_to_time("1970-01-01 00:00:00"));
    auto time_ex2 = message::avp::Time(string_to_time("1972-01-01 00:00:00"));
    auto time_ex3 = message::avp::Time(string_to_time("1999-12-31 00:00:00"));
    auto time_ex4 = message::avp::Time(string_to_time("2036-02-08 00:00:00"));
    auto time_ex5 = message::avp::Time(string_to_time("2262-04-11 23:47:16"));
    //auto time_ex6 = message::avp::Time(string_to_time("2262-04-11 23:47:17")); // std::time_t overflow

    auto time1 = message::avp::Time(string_to_time("2036-02-07 06:28:14"));
    auto time2 = message::avp::Time(string_to_time("2036-02-07 06:28:15"));
    // start era 1
    auto time3 = message::avp::Time(string_to_time("2036-02-07 06:28:16"));
    auto time4 = message::avp::Time(string_to_time("2036-02-07 06:28:17"));
    // ..........
    auto time5 = message::avp::Time(string_to_time("2172-03-15 12:56:30"));
    auto time6 = message::avp::Time(string_to_time("2172-03-15 12:56:31"));
    // start era 2
    auto time7 = message::avp::Time(string_to_time("2172-03-15 12:56:32"));
    auto time8 = message::avp::Time(string_to_time("2172-03-15 12:56:33"));

    BOOST_CHECK_EQUAL(time_ex1->value(), 2'208'988'800);
    BOOST_CHECK_EQUAL(time_ex2->value(), 2'272'060'800);
    BOOST_CHECK_EQUAL(time_ex3->value(), 3'155'587'200);
    BOOST_CHECK_EQUAL(time_ex4->value(), 63'104);
    BOOST_CHECK_EQUAL(time_ex5->value(), 2'842'426'244);

    BOOST_CHECK_EQUAL(time_ex1->era(), 0);
    BOOST_CHECK_EQUAL(time_ex2->era(), 0);
    BOOST_CHECK_EQUAL(time_ex3->era(), 0);
    BOOST_CHECK_EQUAL(time_ex4->era(), 1);
    BOOST_CHECK_EQUAL(time_ex5->era(), 2);

    BOOST_CHECK_EQUAL(time1->value(), 0xFFFFFFFE);
    BOOST_CHECK_EQUAL(time2->value(), 0xFFFFFFFF);
    BOOST_CHECK_EQUAL(time3->value(), 0);
    BOOST_CHECK_EQUAL(time4->value(), 1);

    BOOST_CHECK_EQUAL(time1->era(), 0);
    BOOST_CHECK_EQUAL(time2->era(), 0);
    BOOST_CHECK_EQUAL(time3->era(), 1);
    BOOST_CHECK_EQUAL(time4->era(), 1);

    BOOST_CHECK_EQUAL(time5->value(), 0xFFFFFFFE);
    BOOST_CHECK_EQUAL(time6->value(), 0xFFFFFFFF);
    BOOST_CHECK_EQUAL(time7->value(), 0);
    BOOST_CHECK_EQUAL(time8->value(), 1);

    BOOST_CHECK_EQUAL(time5->era(), 1);
    BOOST_CHECK_EQUAL(time6->era(), 1);
    BOOST_CHECK_EQUAL(time7->era(), 2);
    BOOST_CHECK_EQUAL(time8->era(), 2);

    BOOST_CHECK_EQUAL(time_to_string(time_ex1->to_unix()), "1970-01-01 00:00:00");
    BOOST_CHECK_EQUAL(time_to_string(time_ex2->to_unix()), "1972-01-01 00:00:00");
    BOOST_CHECK_EQUAL(time_to_string(time_ex3->to_unix()), "1999-12-31 00:00:00");
    BOOST_CHECK_EQUAL(time_to_string(time_ex4->to_unix()), "2036-02-08 00:00:00");

    BOOST_CHECK_EQUAL(time_to_string(time1->to_unix()), "2036-02-07 06:28:14");
    BOOST_CHECK_EQUAL(time_to_string(time2->to_unix()), "2036-02-07 06:28:15");
    BOOST_CHECK_EQUAL(time_to_string(time3->to_unix()), "2036-02-07 06:28:16");
    BOOST_CHECK_EQUAL(time_to_string(time4->to_unix()), "2036-02-07 06:28:17");

    BOOST_CHECK_EQUAL(time_to_string(time5->to_unix()), "2172-03-15 12:56:30");
    BOOST_CHECK_EQUAL(time_to_string(time6->to_unix()), "2172-03-15 12:56:31");
    BOOST_CHECK_EQUAL(time_to_string(time7->to_unix()), "2172-03-15 12:56:32");
    BOOST_CHECK_EQUAL(time_to_string(time8->to_unix()), "2172-03-15 12:56:33");
}

BOOST_AUTO_TEST_CASE(constructor_from_ntp)
{
    uint32_t t3 = 3'155'587'200;
    uint32_t t4 = 63'104;
    auto time_ex3 = message::avp::Time(t3);
    auto time_ex4 = message::avp::Time(t4);
    BOOST_CHECK_EQUAL(time_to_string(time_ex3->to_unix()), "1999-12-31 00:00:00");
    BOOST_CHECK_EQUAL(time_to_string(time_ex4->to_unix()), "2036-02-08 00:00:00");
}

BOOST_AUTO_TEST_SUITE_END()

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

BOOST_AUTO_TEST_CASE(unix_epoch_and_wire_size)
{
    const auto time = message::avp::Time(std::chrono::system_clock::from_time_t(0));
    BOOST_CHECK_EQUAL(time->value(), 2'208'988'800u);
    BOOST_CHECK_EQUAL(time->era(), 0);
    BOOST_CHECK_EQUAL(time.size(), 4);
    BOOST_CHECK_EQUAL(std::chrono::system_clock::to_time_t(time->to_unix()), 0);
}

BOOST_AUTO_TEST_CASE(inferred_era_wraps_at_2036_boundary)
{
    const auto before = message::avp::Time(uint32_t{0xffffffff});
    const auto after = message::avp::Time(uint32_t{0});
    BOOST_CHECK_EQUAL(before->era(), 0);
    BOOST_CHECK_EQUAL(after->era(), 1);
    const auto difference = std::chrono::duration_cast<std::chrono::seconds>(
        after->to_unix() - before->to_unix());
    BOOST_CHECK_EQUAL(difference.count(), 1);
    BOOST_CHECK_EQUAL(time_to_string(before->to_unix()), "2036-02-07 06:28:15");
    BOOST_CHECK_EQUAL(time_to_string(after->to_unix()), "2036-02-07 06:28:16");
}

BOOST_AUTO_TEST_CASE(explicit_era_preserves_identical_wire_value)
{
    const auto era0 = message::avp::Time(uint32_t{2'208'988'800u}, uint32_t{0});
    const auto era1 = message::avp::Time(uint32_t{2'208'988'800u}, uint32_t{1});
    BOOST_CHECK_EQUAL(era0->value(), era1->value());
    BOOST_CHECK_EQUAL(era0->era(), 0);
    BOOST_CHECK_EQUAL(era1->era(), 1);
    const auto difference = std::chrono::duration_cast<std::chrono::seconds>(
        era1->to_unix() - era0->to_unix());
    BOOST_CHECK_EQUAL(difference.count(), 4'294'967'296LL);
}

BOOST_AUTO_TEST_CASE(explicit_era_one_supports_2104_half_era_boundary)
{
    const auto before = message::avp::Time(uint32_t{0x7fffffff}, uint32_t{1});
    const auto after = message::avp::Time(uint32_t{0x80000000}, uint32_t{1});
    BOOST_CHECK_EQUAL(time_to_string(before->to_unix()), "2104-02-26 09:42:23");
    BOOST_CHECK_EQUAL(time_to_string(after->to_unix()), "2104-02-26 09:42:24");
    const auto reconstructed = message::avp::Time(after->to_unix());
    BOOST_CHECK_EQUAL(reconstructed->value(), 0x80000000);
    BOOST_CHECK_EQUAL(reconstructed->era(), 1);
}

BOOST_AUTO_TEST_CASE(pre_unix_epoch_time_can_be_represented)
{
    // A negative Unix timestamp is representable by system_clock and by NTP era zero.
    const auto instant = std::chrono::system_clock::from_time_t(-1);
    const auto check_from_unix = [&instant] {
        const auto encoded = message::avp::Time{instant};
        BOOST_CHECK_EQUAL(encoded->value(), 2'208'988'799u);
        BOOST_CHECK_EQUAL(encoded->era(), 0);
    };
    BOOST_CHECK_NO_THROW(check_from_unix());
    const auto ntp = message::avp::Time(uint32_t{2'208'988'799u}, uint32_t{0});
    const auto check_to_unix = [&ntp] {
        const auto decoded = std::chrono::system_clock::to_time_t(ntp->to_unix());
        BOOST_CHECK_EQUAL(decoded, -1);
    };
    BOOST_CHECK_NO_THROW(check_to_unix());
}

BOOST_AUTO_TEST_SUITE_END()

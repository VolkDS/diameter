#ifndef DIAMETER_MESSAGE_AVP_VALUE_TIME_H
#define DIAMETER_MESSAGE_AVP_VALUE_TIME_H

#include <chrono>
#include <iostream>
#include <vector>

namespace diameter::message::avp::value {

/*
 * The Time format is derived from the OctetString Basic AVP Format. The string MUST contain four
 * octets, in the same format as the first four bytes are in the NTP timestamp format.  The NTP
 * timestamp format is defined in Section 3 of [RFC5905].
 *
 * This represents the number of seconds since 0h on 1 January 1900 with respect to the Coordinated
 * Universal Time (UTC).
 *
 * On 6h 28m 16s UTC, 7 February 2036, the time value will overflow. Simple Network Time Protocol
 * (SNTP) [RFC5905] describes a procedure to extend the time to 2104.  This procedure MUST be
 * supported by all Diameter nodes.
 */
class TimeBase
{
public:
    using value_type = uint32_t;
    using unix_t = std::chrono::system_clock::time_point;

    TimeBase(TimeBase const&) = default;
    TimeBase& operator= (TimeBase const&) = default;
    TimeBase(TimeBase&&) = default;
    TimeBase& operator= (TimeBase&&) = default;

    TimeBase(value_type ntp)
        : m_timepoint(ntp)
    {
        m_era = (ntp & 0x8000'0000) ? 0 : 1;
    }

    TimeBase(value_type ntp, uint32_t era)
        : m_timepoint(ntp),
          m_era(era)
    {
    }

    TimeBase(unix_t timepoint)
    {
        std::time_t ts = std::chrono::system_clock::to_time_t(timepoint);
        uint64_t ts64u = static_cast<uint64_t>(ts);
        if (ts64u & 0x8000'0000'0000'0000) {
            // 2262-04-11 23:47:17
            throw std::runtime_error("avp::Time: overflow unixstamp");
        }
        uint64_t ntp = ts64u + m_diff_ntp_to_unix;

        m_era = static_cast<uint32_t>(ntp >> 32);
        m_timepoint = static_cast<uint32_t>(ntp & 0x0000'0000'FFFF'FFFF);
    }

    unix_t to_unix() const
    {
        uint64_t ts64u = static_cast<uint64_t>(m_era) << 32;
        ts64u = ts64u + m_timepoint - m_diff_ntp_to_unix;
        if (ts64u & 0x8000'0000'0000'0000) {
            // 2262-04-11 23:47:17
            throw std::runtime_error("avp::Time: overflow unixstamp");
        }
        std::time_t ts = static_cast<time_t>(ts64u);
        return std::chrono::system_clock::from_time_t(ts);
    }

    value_type value() const
    {
        return m_timepoint;
    }

    uint32_t era() const
    {
        return m_era;
    }

    constexpr size_t size() const
    {
        return sizeof(value_type);
    }

private:
    static constexpr value_type m_diff_ntp_to_unix = static_cast<uint32_t>(70 * 365 + 17) * 86400;

    value_type m_timepoint;
    uint32_t m_era;
};

} // namespace diameter::message::avp::value

#endif

#ifndef DIAMETER_MESSAGE_AVP_VALUE_IP_FILTER_RULE_H
#define DIAMETER_MESSAGE_AVP_VALUE_IP_FILTER_RULE_H

#include <cstdint>
#include <regex>
#include <string>

namespace diameter::message::avp::value {

class IPFilterRuleBase
{
public:
    using value_type = std::string;
    using action_type = std::string;
    using direction_type = std::string;
    using protocol_type = std::string;
    using address_type = std::string;

    IPFilterRuleBase() = default;

    IPFilterRuleBase(IPFilterRuleBase const&) = default;
    IPFilterRuleBase& operator= (IPFilterRuleBase const&) = default;
    IPFilterRuleBase(IPFilterRuleBase&&) = default;
    IPFilterRuleBase& operator= (IPFilterRuleBase&&) = default;

    IPFilterRuleBase(const value_type& val)
        : m_value(val)
    {
    }

    bool validate()
    {
        static const std::regex r1(
            R"(^(permit|deny)\s+(in|out)\s+(ip|tcp|udp|icmp)\s+from\s+(any|(?:\d{1,3}\.){3}\d{1,3}(?:\/\d{1,2})?|(?:[a-fA-F0-9:]+)(?:\/\d{1,3})?)(?:\s+\d{1,5}(?:-\d{1,5})?)?\s+to\s+(any|(?:\d{1,3}\.){3}\d{1,3}(?:\/\d{1,2})?|(?:[a-fA-F0-9:]+)(?:\/\d{1,3})?)(?:\s+\d{1,5}(?:-\d{1,5})?)?$)",
            std::regex_constants::icase);

        std::smatch match;
        m_is_validated = std::regex_match(m_value, match, r1);
        if (!m_is_validated) {
            return m_is_validated;
        }
        for (std::size_t i = 0; i < match.size(); ++i) {
            if (!match[i].matched)
                continue;
            std::string piece = match[i].str();
            switch (i) {
                case 1:
                    m_action = piece;
                    break;
                case 2:
                    m_direction = piece;
                    break;
                case 3:
                    m_protocol = piece;
                    break;
                case 4:
                    m_src = piece;
                    break;
                case 5:
                    m_dst = piece;
                    break;
                default:
                    break;
            }
        }
        return m_is_validated;
    }

    value_type value() const noexcept
    {
        return m_value;
    }

    std::size_t size() const noexcept
    {
        return m_value.size();
    }

    action_type action() const noexcept
    {
        return m_action;
    }

    direction_type direction() const noexcept
    {
        return m_direction;
    }
    protocol_type protocol() const noexcept
    {
        return m_protocol;
    }
    address_type src() const noexcept
    {
        return m_src;
    }
    address_type dst() const noexcept
    {
        return m_dst;
    }

private:
    value_type m_value;

    bool m_is_validated {false};
    action_type m_action;
    direction_type m_direction;
    protocol_type m_protocol;
    address_type m_src;
    address_type m_dst;
};

} // namespace diameter::message::avp::value

#endif

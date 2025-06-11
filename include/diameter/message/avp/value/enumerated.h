#ifndef DIAMETER_MESSAGE_AVP_VALUE_ENUMERATED_H
#define DIAMETER_MESSAGE_AVP_VALUE_ENUMERATED_H

#include <cstdint>
#include <regex>
#include <string>

namespace diameter::message::avp::value {

/* The Enumerated format is derived from the Integer32 Basic AVP Format. The definition contains a
 * list of valid values and their interpretation and is described in the Diameter application
 * introducing the AVP.
 */
class EnumeratedBase
{
public:
    using value_type = int32_t;

    EnumeratedBase() = default;

    EnumeratedBase(EnumeratedBase const&) = default;
    EnumeratedBase& operator= (EnumeratedBase const&) = default;
    EnumeratedBase(EnumeratedBase&&) = default;
    EnumeratedBase& operator= (EnumeratedBase&&) = default;

    EnumeratedBase(value_type val)
        : m_value(val)
    {
    }

    value_type value() const noexcept
    {
        return m_value;
    }

    constexpr std::size_t size() const noexcept
    {
        return sizeof(value_type);
    }

private:
    value_type m_value;
};

}

#endif

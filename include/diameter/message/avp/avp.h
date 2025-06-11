#ifndef DIAMETER_MESSAGE_AVP_AVP_H
#define DIAMETER_MESSAGE_AVP_AVP_H

#include <any>
#include <cstdint>
#include <list>
#include <numeric>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

#include <netpacker/netpacker.h>

#include <diameter/message/avp/code.h>
#include <diameter/message/avp/flags.h>
#include <diameter/message/avp/length.h>
#include <diameter/message/avp/value/address.h>
#include <diameter/message/avp/value/diameter_identity.h>
#include <diameter/message/avp/value/diameter_uri.h>
#include <diameter/message/avp/value/enumerated.h>
#include <diameter/message/avp/value/ip_filter_rule.h>
#include <diameter/message/avp/value/time.h>
#include <diameter/message/avp/vendor_id.h>

namespace diameter::message::avp {

struct AVP;

template<typename RawValue>
class BasicValue;

template<typename T>
struct is_basic_value : std::false_type
{
    using type = T;
};

template<typename T>
struct is_basic_value<BasicValue<T>> : std::true_type
{
    using type = BasicValue<T>;
};

template<typename RawValue>
class BasicValue
{
public:
    using value_type = RawValue;

    BasicValue() = default;

    template<typename ... Args,
             typename = std::enable_if_t<
             !std::disjunction_v<is_basic_value<std::decay_t<Args>>...>
             >>
    BasicValue(Args&&... args)
        : m_value(std::forward<decltype(args)>(args)...)
    {
    }

    BasicValue(BasicValue const&) = default;
    BasicValue& operator= (BasicValue const&) = default;
    BasicValue(BasicValue&&) = default;
    BasicValue& operator= (BasicValue&&) = default;

    value_type& operator* () noexcept
    {
        return m_value;
    }
    value_type* operator->() noexcept
    {
        return &m_value;
    }

    value_type const& operator* () const noexcept
    {
        return m_value;
    }
    value_type const* operator->() const noexcept
    {
        return &m_value;
    }

    value_type* get() const noexcept
    {
        return &m_value;
    }

    value_type data() const noexcept
    {
        return m_value;
    }

    Length size() const;

private:
    value_type m_value;
};

// RFC 6733 4.2 Basic AVP Data Formats
using OctetString = BasicValue<std::vector<uint8_t>>;
using Integer32 = BasicValue<int32_t>;
using Integer64 = BasicValue<int64_t>;
using Unsigned32 = BasicValue<uint32_t>;
using Unsigned64 = BasicValue<uint64_t>;
using Float32 = BasicValue<float>;
using Float64 = BasicValue<double>;
// RFC 6733 4.3 Derived AVP Data Formats
using Address = BasicValue<value::AddressBase>;
using Time = BasicValue<value::TimeBase>;
using UTF8String = BasicValue<std::string>;
using DiameterIdentity = BasicValue<value::DiameterIdentityBase>;
using DiameterURI = BasicValue<value::DiameterURIBase>;
using Enumerated = BasicValue<value::EnumeratedBase>;
using IPFilterRule = BasicValue<value::IPFilterRuleBase>;
// RFC 6733 4.4 Grouped AVP Values
using Grouped = BasicValue<std::list<AVP>>;

using Value = std::variant<OctetString, Integer32, Integer64, Unsigned32, Unsigned64, Float32,
    Float64, Address, Time, UTF8String, DiameterIdentity, DiameterURI, Enumerated, IPFilterRule,
    Grouped>;

template<typename RawValue>
Length BasicValue<RawValue>::size() const
{
    if constexpr (std::is_arithmetic_v<RawValue>) {
        return static_cast<Length>(sizeof(RawValue));
    }
    else if constexpr (std::is_same_v<BasicValue<RawValue>, std::decay_t<Grouped>>) {
        Length len = 0;
        for (const auto& avp : m_value) {
            len += avp.size();
        }
        return len;
        // Length len = 0;
        // len = std::accumulate(m_value.begin(), m_value.end(), len, [](Length sum, auto&& avp){
        //     return std::move(sum) + avp.size();
        // });
        // return len;
    }
    else {
        return static_cast<Length>(m_value.size());
    }
    // throw std::runtime_error("BasicValue::cast: unsupported cast");
}

struct AVP
{
    Code code;
    Flags flags;
    // Length length;
    std::optional<VendorId> vendor_id;
    Value value;

    AVP() = default;

    AVP(AVP const&) = default;
    AVP& operator= (AVP const&) = default;
    AVP(AVP&&) = default;
    AVP& operator= (AVP&&) = default;

    Length length() const
    {
        Length len = sizeof(Code) + sizeof(Flags::UnderlyingT) + sizeof(Length) - 1;
        if (vendor_id.has_value()) {
            len += sizeof(VendorId);
        }

        len += std::visit(
            [](auto&& arg) -> Length {
                return arg.size();
            },
            value);

        return len;
    }

    Length size() const
    {
        return length() + padding();
    }

    Length padding() const
    {
        auto len = length();
        uint32_t pad = len % 4;
        pad = pad ? 4 - pad : 0;
        return pad;
    }
};

} // namespace diameter::message::avp

#endif

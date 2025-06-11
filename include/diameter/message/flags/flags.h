#ifndef DIAMETER_MESSAGE_FLAGS_FLAGS_H
#define DIAMETER_MESSAGE_FLAGS_FLAGS_H

#include <bitset>
#include <cstddef>
#include <stdexcept>
#include <type_traits>

namespace diameter::message::flags {

template<typename EnumT, std::size_t N>
class Flags
{
    static_assert(std::is_enum_v<EnumT>, "Flags can only be specialized for enum types");

public:
    using UnderlyingT = typename std::make_unsigned_t<typename std::underlying_type_t<EnumT>>;

    Flags() = default;

    Flags(const Flags&) = default;
    Flags& operator= (const Flags&) = default;
    Flags(Flags&&) = default;
    Flags& operator= (Flags&&) = default;

    Flags(UnderlyingT value)
        : bits_ {value}
    {
    }

    Flags(EnumT e)
    {
        bits_.set(underlying(e), true);
    }

    Flags& set(EnumT e, bool value = true) noexcept
    {
        bits_.set(underlying(e), value);
        return *this;
    }

    Flags& reset(EnumT e) noexcept
    {
        set(e, false);
        return *this;
    }

    Flags& reset() noexcept
    {
        bits_.reset();
        return *this;
    }

    bool all() const noexcept
    {
        return bits_.all();
    }

    bool any() const noexcept
    {
        return bits_.any();
    }

    bool none() const noexcept
    {
        return bits_.none();
    }

    constexpr std::size_t size() const
    {
        if constexpr (N <= 8) {
            return sizeof(uint8_t);
        }
        else if constexpr (N <= 16) {
            return sizeof(uint16_t);
        }
        else if constexpr (N <= 32) {
            return sizeof(uint32_t);
        }
        else if constexpr (N <= 64) {
            return sizeof(uint64_t);
        }
        else {
            throw std::runtime_error("Too big flag value");
        }
    }

    std::size_t count() const noexcept
    {
        return bits_.count();
    }

    uint64_t to_ullong() const noexcept
    {
        return bits_.to_ullong();
    }

    UnderlyingT data() const noexcept
    {
        return static_cast<UnderlyingT>(bits_.to_ullong());
    }

    constexpr bool operator[] (EnumT e) const
    {
        return bits_[underlying(e)];
    }

    Flags operator| (const Flags& other) const noexcept
    {
        return Flags(bits_ | other.bits_);
    }

    Flags operator& (const Flags& other) const noexcept
    {
        return Flags(bits_ & other.bits_);
    }

    Flags operator^ (const Flags& other) const noexcept
    {
        return Flags(bits_ ^ other.bits_);
    }

private:
    Flags(std::bitset<N> value)
        : bits_ {value}
    {
    }

    static constexpr UnderlyingT underlying(EnumT e)
    {
        return static_cast<UnderlyingT>(e);
    }

private:
    std::bitset<N> bits_;
};

} // namespace diameter::message::flags

#endif

#ifndef DIAMETER_SERIAL_ERROR_H
#define DIAMETER_SERIAL_ERROR_H

#include <exception>
#include <string>

namespace diameter::serial {

class Exception : public std::exception
{
protected:
    Exception() noexcept = default;

public:
    virtual ~Exception() = default;
};

class InvalidProtocolVersion : public Exception
{
public:
    InvalidProtocolVersion() noexcept = default;

    const char* what() const noexcept override
    {
        return "Invalid protocol version";
    }
};

class InvalidMessageLength : public Exception
{
public:
    InvalidMessageLength() noexcept = default;

    const char* what() const noexcept override
    {
        return "Invalid message length";
    }
};

class InvalidAvpLength : public Exception
{
public:
    explicit InvalidAvpLength(const std::string& message)
        : what_message(message) {};

    const char* what() const noexcept override
    {
        return what_message.c_str();
    }

private:
    std::string what_message;
};

class InvalidAvpVendorId : public Exception
{
public:
    InvalidAvpVendorId() noexcept = default;

    const char* what() const noexcept override
    {
        return "Invalid avp: No vendor_id in vendor specific avp";
    }
};

class InvalidAvpValueCast : public Exception
{
public:
    explicit InvalidAvpValueCast(const std::string& message)
        : what_message(message) {};

    const char* what() const noexcept override
    {
        return what_message.c_str();
    }

private:
    std::string what_message;
};

} // namespace diameter::serial

#endif

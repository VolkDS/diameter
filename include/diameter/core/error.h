#ifndef DIAMETER_CORE_ERROR_H
#define DIAMETER_CORE_ERROR_H

#include <exception>
#include <string>

namespace diameter::core {

enum class Error : uint32_t
{
    Success = 0,
    DecodingError = 1,
    EncodingError = 2,
    NetworkError = 3,
    Timeout = 4
};

class Exception : public std::exception
{
protected:
    Exception() noexcept = default;

public:
    virtual ~Exception() = default;
};

class AvpOccursTooManyTimes : public Exception
{
public:
    explicit AvpOccursTooManyTimes(const std::string& message)
        : what_message(message) {};

    const char* what() const noexcept override
    {
        return what_message.c_str();
    }

private:
    std::string what_message;
};

class MissingAvp : public Exception
{
public:
    explicit MissingAvp(const std::string& message)
        : what_message(message) {};

    const char* what() const noexcept override
    {
        return what_message.c_str();
    }

private:
    std::string what_message;
};

} // namespace diameter::core

#endif
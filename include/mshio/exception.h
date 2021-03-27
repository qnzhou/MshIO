#pragma once

#include <exception>
#include <string>

namespace mshio {

struct InvalidFormat : public std::exception
{
public:
    InvalidFormat(const std::string& message)
        : m_message(message)
    {}

    const char* what() const noexcept override { return m_message.c_str(); }

private:
    std::string m_message;
};

struct UnsupportedFeature : public std::exception
{
public:
    UnsupportedFeature(const std::string& message)
        : m_message(message)
    {}

    const char* what() const noexcept override { return m_message.c_str(); }

private:
    std::string m_message;
};

struct CorruptData : public std::exception
{
public:
    CorruptData(const std::string& message)
        : m_message(message)
    {}

    const char* what() const noexcept override { return m_message.c_str(); }

private:
    std::string m_message;
};

} // namespace mshio

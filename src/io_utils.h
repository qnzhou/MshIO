#pragma once

#include <mshio/exception.h>
#include <cctype>
#include <cstdint>
#include <istream>
#include <limits>
#include <ostream>
#include <type_traits>
#include <vector>

namespace mshio {

void eat_white_space(std::istream& in, size_t count = std::numeric_limits<size_t>::max());

inline size_t checked_size(std::uint64_t count)
{
    if (count > std::numeric_limits<size_t>::max())
        throw InvalidFormat("MSH count exceeds host allocation limits");
    return static_cast<size_t>(count);
}

inline std::uint64_t checked_product(std::uint64_t count, std::uint64_t stride)
{
    if (stride != 0 && count > std::numeric_limits<std::uint64_t>::max() / stride)
        throw InvalidFormat("MSH count/stride overflow");
    return count * stride;
}

inline std::uint64_t checked_sum(std::uint64_t a, std::uint64_t b)
{
    if (b > std::numeric_limits<std::uint64_t>::max() - a)
        throw InvalidFormat("MSH count overflow");
    return a + b;
}

inline std::streamsize checked_bytes(std::uint64_t count, size_t width)
{
    const auto bytes = checked_product(count, width);
    checked_size(bytes);
    if (bytes > static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max()))
        throw InvalidFormat("MSH byte count exceeds stream limits");
    return static_cast<std::streamsize>(bytes);
}

template <typename T>
size_t checked_count(const std::vector<T>& values, std::uint64_t count)
{
    const auto n = checked_size(count);
    if (n > values.max_size()) throw InvalidFormat("MSH count exceeds container limits");
    checked_bytes(count, sizeof(T));
    return n;
}

template <typename T>
void resize_checked(std::vector<T>& values, std::uint64_t count)
{
    values.resize(checked_count(values, count));
}

inline std::uint64_t nonnegative(std::int32_t value)
{
    if (value < 0) throw InvalidFormat("Negative MSH count or identifier");
    return static_cast<std::uint64_t>(value);
}

inline std::int32_t checked_int32(std::uint64_t value)
{
    if (value > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()))
        throw InvalidFormat("MSH 2.2 field exceeds signed 32-bit range");
    return static_cast<std::int32_t>(value);
}

template <typename T>
typename std::enable_if<std::is_integral<T>::value>::type read_ascii(std::istream& in, T& value)
{
    in >> std::ws;
    bool negative = false;
    int ch = in.peek();
    if (ch == '-' || ch == '+') {
        negative = ch == '-';
        in.get();
        ch = in.peek();
    }
    if (negative && !std::is_signed<T>::value)
        throw InvalidFormat("Negative unsigned MSH field");
    if (ch < '0' || ch > '9') throw InvalidFormat("Invalid or truncated MSH integer");
    const auto limit = static_cast<std::uint64_t>(std::numeric_limits<T>::max()) +
        (negative ? 1u : 0u);
    std::uint64_t magnitude = 0;
    do {
        const auto digit = static_cast<unsigned>(ch - '0');
        if (magnitude > limit / 10 || (magnitude == limit / 10 && digit > limit % 10))
            throw InvalidFormat("MSH integer is out of range");
        magnitude = magnitude * 10 + digit;
        in.get();
        ch = in.peek();
    } while (ch >= '0' && ch <= '9');
    if (ch != std::char_traits<char>::eof() && !std::isspace(static_cast<unsigned char>(ch)))
        throw InvalidFormat("Invalid MSH integer token");
    if (negative) {
        value = magnitude == limit ? std::numeric_limits<T>::min() :
            static_cast<T>(-static_cast<std::int64_t>(magnitude));
    } else {
        value = static_cast<T>(magnitude);
    }
}

inline void read_ascii(std::istream& in, double& value)
{
    if (!(in >> value)) throw InvalidFormat("Invalid or truncated MSH floating-point field");
}

template <typename T>
void read_binary(std::istream& in, T* values, std::uint64_t count = 1)
{
    const auto bytes = checked_bytes(count, sizeof(T));
    if (bytes && !in.read(reinterpret_cast<char*>(values), bytes))
        throw InvalidFormat("Truncated MSH binary data");
}

template <typename T>
void write_binary(std::ostream& out, const T* values, std::uint64_t count = 1)
{
    const auto bytes = checked_bytes(count, sizeof(T));
    if (bytes && !out.write(reinterpret_cast<const char*>(values), bytes))
        throw InvalidFormat("Failed to write MSH binary data");
}

template <typename T>
void read_field(std::istream& in, T& value, bool binary)
{
    if (binary) read_binary(in, &value);
    else read_ascii(in, value);
}

template <typename T>
void read_values(std::istream& in, std::vector<T>& values, std::uint64_t count, bool binary)
{
    resize_checked(values, count);
    if (binary) read_binary(in, values.data(), values.size());
    else for (auto& value : values) read_ascii(in, value);
}

inline size_t node_stride(std::int32_t dim, std::int32_t parametric)
{
    if (dim < 0 || dim > 3 || (parametric != 0 && parametric != 1))
        throw InvalidFormat("Invalid MSH node dimension or parametric flag");
    return static_cast<size_t>(3 + (parametric ? dim : 0));
}

} // namespace mshio

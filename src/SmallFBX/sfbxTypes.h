#pragma once

#include <cstdint>
#ifdef __cpp_lib_span
    #include <span>
#endif

namespace sfbx {

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using float32 = float;
using float64 = double;

#ifdef __cpp_lib_span

template<class T> using span = std::span<T>;

#else

// equivalent of std::span in C++20
template<class T>
class span
{
public:
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = pointer;
    using const_iterator = const_pointer;

    span() {}
    span(const T* d, size_t s) : m_data(const_cast<T*>(d)), m_size(s) {}
    span(const span& v) : m_data(const_cast<T*>(v.m_data)), m_size(v.m_size) {}
    span& operator=(const span& v) { m_data = const_cast<T*>(v.m_data); m_size = v.m_size; return *this; }

    bool empty() const { return m_size == 0; }
    size_t size() const { return m_size; }
    size_t size_bytes() const { return sizeof(T) * m_size; }

    T* data() { return m_data; }
    const T* data() const { return m_data; }

    T& operator[](size_t i) { return m_data[i]; }
    const T& operator[](size_t i) const { return m_data[i]; }

    T& front() { return m_data[0]; }
    const T& front() const { return m_data[0]; }
    T& back() { return m_data[m_size - 1]; }
    const T& back() const { return m_data[m_size - 1]; }

    iterator begin() { return m_data; }
    const_iterator begin() const { return m_data; }
    iterator end() { return m_data + m_size; }
    const_iterator end() const { return m_data + m_size; }

private:
    T* m_data = nullptr;
    size_t m_size = 0;
};
#endif

} // namespace sfbx

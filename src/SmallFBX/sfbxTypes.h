#pragma once

#include <cstdint>
#include <vector>
#include <type_traits>
#ifdef __cpp_lib_span
    #include <span>
#endif

#define sfbxEnableIf(...) std::enable_if_t<__VA_ARGS__, bool> = true

namespace sfbx {

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using float32 = float;
using float64 = double;

template<class T>
struct tvec2
{
    T x, y;
    T& operator[](int i) { return ((T*)this)[i]; }
    const T& operator[](int i) const { return ((T*)this)[i]; }
    template<class U> operator tvec2<U>() const { return { (U)x, (U)y }; }
};

template<class T>
struct tvec3
{
    T x, y, z;
    T& operator[](int i) { return ((T*)this)[i]; }
    const T& operator[](int i) const { return ((T*)this)[i]; }
    template<class U> operator tvec3<U>() const { return { (U)x, (U)y, (U)z }; }
};

template<class T>
struct tvec4
{
    T x, y, z, w;
    T& operator[](int i) { return ((T*)this)[i]; }
    const T& operator[](int i) const { return ((T*)this)[i]; }
    template<class U> operator tvec4<U>() const { return { (U)x, (U)y, (U)z, (U)w }; }
};

template<class T>
struct tmat4x4
{
    tvec4<T> m[4];
    tvec4<T>& operator[](int i) { return m[i]; }
    const tvec4<T>& operator[](int i) const { return m[i]; }
    template<class U> operator tmat4x4<U>() const { return { { tvec4<U>(m[0]), tvec4<U>(m[1]), tvec4<U>(m[2]), tvec4<U>(m[3]) } }; }
};

using int2 = tvec2<int>;
using int3 = tvec3<int>;
using int4 = tvec4<int>;

using float2 = tvec2<float>;
using float3 = tvec3<float>;
using float4 = tvec4<float>;
using quatf = tvec4<float>;
using float4x4 = tmat4x4<float>;

using double2 = tvec2<double>;
using double3 = tvec3<double>;
using double4 = tvec4<double>;
using quatd = tvec4<double>;
using double4x4 = tmat4x4<double>;

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

template<class T> inline span<T> make_span(const std::vector<T>& v) { return { (T*)v.data(), v.size() }; }
template<class T> inline span<T> make_span(const T* v, size_t n) { return { (T*)v, n }; }


#define Decl(T) class T; using T##Ptr = std::shared_ptr<T>;
Decl(Property)
Decl(Node)

Decl(Object)
Decl(Model)
Decl(Geometry)
Decl(Deformer)
Decl(Pose)
Decl(Material)

Decl(Document)
#undef Decl

} // namespace sfbx

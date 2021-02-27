#pragma once
#include <iostream>
#include "sfbxTypes.h"
#include "sfbxRawVector.h"

#define sfbxReturnIfNull() if (!this) { return {}; }

namespace sfbx {

template<class T>
inline T read1(std::istream& is)
{
    static_assert(std::is_pod_v<T>);
    T r;
    is.read((char*)&r, sizeof(T));
    return r;
}

inline void readv(std::istream& is, void* dst, size_t size)
{
    is.read((char*)dst, size);
}

inline void readv(std::istream& is, std::string& dst, size_t s)
{
    dst.resize(s);
    is.read(dst.data(), s);
}

template<class T>
inline void write1(std::ostream& os, T v)
{
    static_assert(std::is_pod_v<T>);
    os.write((const char*)&v, sizeof(T));
}

inline void writev(std::ostream& os, const void* src, size_t size)
{
    os.write((const char*)src, size);
}


template<class Values, class Body>
void each(Values& val, const Body& body)
{
    for (auto& v : val)
        body(v);
}

template<class Values, class Indices, class Body>
void each_indexed(Values& val, Indices& idx, const Body& body)
{
    for (auto i : idx)
        body(val[i]);
}

template<class Dst, class Src>
void copy(Dst& dst, const Src& src)
{
    dst.resize(src.size());
    auto* d = dst.data();
    for (const auto& v : src)
        *d++ = v;
}
template<class T, class U>
void copy(span<T> dst, const span<U> src)
{
    auto* d = dst.data();
    for (const U& v : src)
        *d++ = v;
}

template<class Dst, class Src, class Indices>
void copy_indexed(Dst& dst, Src& src, Indices& idx)
{
    dst.resize(idx.size());
    auto* d = dst.data();
    for (auto i : idx)
        *d++ = src[i];
}

} // namespace sfbx

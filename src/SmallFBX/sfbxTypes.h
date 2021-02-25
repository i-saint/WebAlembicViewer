#pragma once

#include <cstdint>

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

using float2 = tvec2<float>;
using float3 = tvec3<float>;
using float4 = tvec4<float>;

using double2 = tvec2<double>;
using double3 = tvec3<double>;
using double4 = tvec4<double>;

#define Decl(T) class T; using T##Ptr = std::shared_ptr<T>;
Decl(Property)
Decl(Node)
Decl(Document)
#undef Decl

} // namespace sfbx

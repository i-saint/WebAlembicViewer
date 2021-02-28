#pragma once
#include "sfbxTypes.h"

namespace sfbx {

template<class T> inline tvec2<T> operator-(const tvec2<T>& v) { return{ -v.x, -v.y }; }
template<class T, class U> inline tvec2<T> operator+(const tvec2<T>& l, const tvec2<U>& r) { return{ l.x + r.x, l.y + r.y }; }
template<class T, class U> inline tvec2<T> operator-(const tvec2<T>& l, const tvec2<U>& r) { return{ l.x - r.x, l.y - r.y }; }
template<class T, class U> inline tvec2<T> operator*(const tvec2<T>& l, const tvec2<U>& r) { return{ l.x * r.x, l.y * r.y }; }
template<class T, class U> inline tvec2<T> operator/(const tvec2<T>& l, const tvec2<U>& r) { return{ l.x / r.x, l.y / r.y }; }
template<class T> inline tvec2<T> operator+(T l, const tvec2<T>& r) { return{ l + r.x, l + r.y }; }
template<class T> inline tvec2<T> operator-(T l, const tvec2<T>& r) { return{ l - r.x, l - r.y }; }
template<class T> inline tvec2<T> operator*(T l, const tvec2<T>& r) { return{ l * r.x, l * r.y }; }
template<class T> inline tvec2<T> operator/(T l, const tvec2<T>& r) { return{ l / r.x, l / r.y }; }
template<class T> inline tvec2<T> operator+(const tvec2<T>& l, T r) { return{ l.x + r, l.y + r }; }
template<class T> inline tvec2<T> operator-(const tvec2<T>& l, T r) { return{ l.x - r, l.y - r }; }
template<class T> inline tvec2<T> operator*(const tvec2<T>& l, T r) { return{ l.x * r, l.y * r }; }
template<class T> inline tvec2<T> operator/(const tvec2<T>& l, T r) { return{ l.x / r, l.y / r }; }
template<class T, class U> inline tvec2<T>& operator+=(tvec2<T>& l, const tvec2<U>& r) { l.x += r.x; l.y += r.y; return l; }
template<class T, class U> inline tvec2<T>& operator-=(tvec2<T>& l, const tvec2<U>& r) { l.x -= r.x; l.y -= r.y; return l; }
template<class T, class U> inline tvec2<T>& operator*=(tvec2<T>& l, const tvec2<U>& r) { l.x *= r.x; l.y *= r.y; return l; }
template<class T, class U> inline tvec2<T>& operator/=(tvec2<T>& l, const tvec2<U>& r) { l.x /= r.x; l.y /= r.y; return l; }
template<class T> inline tvec2<T>& operator+=(tvec2<T>& l, T r) { l.x += r; l.y += r; return l; }
template<class T> inline tvec2<T>& operator-=(tvec2<T>& l, T r) { l.x -= r; l.y -= r; return l; }
template<class T> inline tvec2<T>& operator*=(tvec2<T>& l, T r) { l.x *= r; l.y *= r; return l; }
template<class T> inline tvec2<T>& operator/=(tvec2<T>& l, T r) { l.x /= r; l.y /= r; return l; }

template<class T> inline tvec3<T> operator-(const tvec3<T>& v) { return{ -v.x, -v.y, -v.z }; }
template<class T, class U> inline tvec3<T> operator+(const tvec3<T>& l, const tvec3<U>& r) { return{ l.x + r.x, l.y + r.y, l.z + r.z }; }
template<class T, class U> inline tvec3<T> operator-(const tvec3<T>& l, const tvec3<U>& r) { return{ l.x - r.x, l.y - r.y, l.z - r.z }; }
template<class T, class U> inline tvec3<T> operator*(const tvec3<T>& l, const tvec3<U>& r) { return{ l.x * r.x, l.y * r.y, l.z * r.z }; }
template<class T, class U> inline tvec3<T> operator/(const tvec3<T>& l, const tvec3<U>& r) { return{ l.x / r.x, l.y / r.y, l.z / r.z }; }
template<class T> inline tvec3<T> operator+(T l, const tvec3<T>& r) { return{ l + r.x, l + r.y, l + r.z }; }
template<class T> inline tvec3<T> operator-(T l, const tvec3<T>& r) { return{ l - r.x, l - r.y, l - r.z }; }
template<class T> inline tvec3<T> operator*(T l, const tvec3<T>& r) { return{ l * r.x, l * r.y, l * r.z }; }
template<class T> inline tvec3<T> operator/(T l, const tvec3<T>& r) { return{ l / r.x, l / r.y, l / r.z }; }
template<class T> inline tvec3<T> operator+(const tvec3<T>& l, T r) { return{ l.x + r, l.y + r, l.z + r }; }
template<class T> inline tvec3<T> operator-(const tvec3<T>& l, T r) { return{ l.x - r, l.y - r, l.z - r }; }
template<class T> inline tvec3<T> operator*(const tvec3<T>& l, T r) { return{ l.x * r, l.y * r, l.z * r }; }
template<class T> inline tvec3<T> operator/(const tvec3<T>& l, T r) { return{ l.x / r, l.y / r, l.z / r }; }
template<class T, class U> inline tvec3<T>& operator+=(tvec3<T>& l, const tvec3<U>& r) { l.x += r.x; l.y += r.y; l.z += r.z; return l; }
template<class T, class U> inline tvec3<T>& operator-=(tvec3<T>& l, const tvec3<U>& r) { l.x -= r.x; l.y -= r.y; l.z -= r.z; return l; }
template<class T, class U> inline tvec3<T>& operator*=(tvec3<T>& l, const tvec3<U>& r) { l.x *= r.x; l.y *= r.y; l.z *= r.z; return l; }
template<class T, class U> inline tvec3<T>& operator/=(tvec3<T>& l, const tvec3<U>& r) { l.x /= r.x; l.y /= r.y; l.z /= r.z; return l; }
template<class T> inline tvec3<T>& operator+=(tvec3<T>& l, T r) { l.x += r; l.y += r; l.z += r; return l; }
template<class T> inline tvec3<T>& operator-=(tvec3<T>& l, T r) { l.x -= r; l.y -= r; l.z -= r; return l; }
template<class T> inline tvec3<T>& operator*=(tvec3<T>& l, T r) { l.x *= r; l.y *= r; l.z *= r; return l; }
template<class T> inline tvec3<T>& operator/=(tvec3<T>& l, T r) { l.x /= r; l.y /= r; l.z /= r; return l; }

template<class T> inline tvec4<T> operator-(const tvec4<T>& v) { return{ -v.x, -v.y, -v.z, -v.w }; }
template<class T, class U> inline tvec4<T> operator+(const tvec4<T>& l, const tvec4<U>& r) { return{ l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w }; }
template<class T, class U> inline tvec4<T> operator-(const tvec4<T>& l, const tvec4<U>& r) { return{ l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w }; }
template<class T, class U> inline tvec4<T> operator*(const tvec4<T>& l, const tvec4<U>& r) { return{ l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w }; }
template<class T, class U> inline tvec4<T> operator/(const tvec4<T>& l, const tvec4<U>& r) { return{ l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w }; }
template<class T> inline tvec4<T> operator+(T l, const tvec4<T>& r) { return{ l + r.x, l + r.y, l + r.z, l + r.w }; }
template<class T> inline tvec4<T> operator-(T l, const tvec4<T>& r) { return{ l - r.x, l - r.y, l - r.z, l - r.w }; }
template<class T> inline tvec4<T> operator*(T l, const tvec4<T>& r) { return{ l * r.x, l * r.y, l * r.z, l * r.w }; }
template<class T> inline tvec4<T> operator/(T l, const tvec4<T>& r) { return{ l / r.x, l / r.y, l / r.z, l / r.w }; }
template<class T> inline tvec4<T> operator+(const tvec4<T>& l, T r) { return{ l.x + r, l.y + r, l.z + r, l.w + r }; }
template<class T> inline tvec4<T> operator-(const tvec4<T>& l, T r) { return{ l.x - r, l.y - r, l.z - r, l.w - r }; }
template<class T> inline tvec4<T> operator*(const tvec4<T>& l, T r) { return{ l.x * r, l.y * r, l.z * r, l.w * r }; }
template<class T> inline tvec4<T> operator/(const tvec4<T>& l, T r) { return{ l.x / r, l.y / r, l.z / r, l.w / r }; }
template<class T, class U> inline tvec4<T>& operator+=(tvec4<T>& l, const tvec4<U>& r) { l.x += r.x; l.y += r.y; l.z += r.z; l.w += r.w; return l; }
template<class T, class U> inline tvec4<T>& operator-=(tvec4<T>& l, const tvec4<U>& r) { l.x -= r.x; l.y -= r.y; l.z -= r.z; l.w -= r.w; return l; }
template<class T, class U> inline tvec4<T>& operator*=(tvec4<T>& l, const tvec4<U>& r) { l.x *= r.x; l.y *= r.y; l.z *= r.z; l.w *= r.w; return l; }
template<class T, class U> inline tvec4<T>& operator/=(tvec4<T>& l, const tvec4<U>& r) { l.x /= r.x; l.y /= r.y; l.z /= r.z; l.w /= r.w; return l; }
template<class T> inline tvec4<T>& operator+=(tvec4<T>& l, T r) { l.x += r; l.y += r; l.z += r; l.w += r; return l; }
template<class T> inline tvec4<T>& operator-=(tvec4<T>& l, T r) { l.x -= r; l.y -= r; l.z -= r; l.w -= r; return l; }
template<class T> inline tvec4<T>& operator*=(tvec4<T>& l, T r) { l.x *= r; l.y *= r; l.z *= r; l.w *= r; return l; }
template<class T> inline tvec4<T>& operator/=(tvec4<T>& l, T r) { l.x /= r; l.y /= r; l.z /= r; l.w /= r; return l; }

template<class T> inline tquat<T> operator*(const tquat<T>& l, T r) { return{ l.x * r, l.y * r, l.z * r, l.w * r }; }
template<class T> inline tquat<T> operator*(const tquat<T>& l, const tquat<T>& r)
{
    return{
        l.w * r.x + l.x * r.w + l.y * r.z - l.z * r.y,
        l.w * r.y + l.y * r.w + l.z * r.x - l.x * r.z,
        l.w * r.z + l.z * r.w + l.x * r.y - l.y * r.x,
        l.w * r.w - l.x * r.x - l.y * r.y - l.z * r.z,
    };
}
template<class T> inline tquat<T>& operator*=(tquat<T>& l, T r)
{
    l = l * r;
    return l;
}
template<class T> inline tquat<T>& operator*=(tquat<T>& l, const tquat<T>& r)
{
    l = l * r;
    return l;
}

template<class T> inline T sum(const tvec2<T>& v) { return v.x + v.y; }
template<class T> inline T sum(const tvec3<T>& v) { return v.x + v.y + v.z; }
template<class T> inline T sum(const tvec4<T>& v) { return v.x + v.y + v.z + v.w; }

template<class T> inline tquat<T> rotate_x(T angle)
{
    T c = cos(angle * T(0.5));
    T s = sin(angle * T(0.5));
    return{ s, T(0.0), T(0.0), c };
}
template<class T> inline tquat<T> rotate_y(T angle)
{
    T c = cos(angle * T(0.5));
    T s = sin(angle * T(0.5));
    return{ T(0.0), s, T(0.0), c };
}
template<class T> inline tquat<T> rotate_z(T angle)
{
    T c = cos(angle * T(0.5));
    T s = sin(angle * T(0.5));
    return{ T(0.0), T(0.0), s, c };
}

// euler -> quaternion
template<class T> inline tquat<T> rotate_xyz(const tvec3<T>& euler)
{
    auto rx = rotate_x(euler.x);
    auto ry = rotate_y(euler.y);
    auto rz = rotate_z(euler.z);
    return (rz * ry) * rx;
}
template<class T> inline tquat<T> rotate_xzy(const tvec3<T>& euler)
{
    auto rx = rotate_x(euler.x);
    auto ry = rotate_y(euler.y);
    auto rz = rotate_z(euler.z);
    return (ry * rz) * rx;
}
template<class T> inline tquat<T> rotate_yxz(const tvec3<T>& euler)
{
    auto rx = rotate_x(euler.x);
    auto ry = rotate_y(euler.y);
    auto rz = rotate_z(euler.z);
    return (rz * rx) * ry;
}
template<class T> inline tquat<T> rotate_yzx(const tvec3<T>& euler)
{
    auto rx = rotate_x(euler.x);
    auto ry = rotate_y(euler.y);
    auto rz = rotate_z(euler.z);
    return (rx * rz) * ry;
}
template<class T> inline tquat<T> rotate_zxy(const tvec3<T>& euler)
{
    auto rx = rotate_x(euler.x);
    auto ry = rotate_y(euler.y);
    auto rz = rotate_z(euler.z);
    return (ry * rx) * rz;
}
template<class T> inline tquat<T> rotate_zyx(const tvec3<T>& euler)
{
    auto rx = rotate_x(euler.x);
    auto ry = rotate_y(euler.y);
    auto rz = rotate_z(euler.z);
    return (rx * ry) * rz;
}

template<class T> inline tquat<T> rotate_euler(RotationOrder order, const tvec3<T>& euler)
{
    switch (order)
    {
    case RotationOrder::XYZ: return rotate_xyz(euler);
    case RotationOrder::XZY: return rotate_xzy(euler);
    case RotationOrder::YZX: return rotate_yzx(euler);
    case RotationOrder::YXZ: return rotate_yxz(euler);
    case RotationOrder::ZXY: return rotate_zxy(euler);
    case RotationOrder::ZYX: return rotate_zyx(euler);
    case RotationOrder::SphericXYZ: return rotate_xyz(euler); // todo
    default: return tquat<T>::identity();
    }
}

template<class T> inline tmat4x4<T> to_mat4x4(const tquat<T>& v)
{
    return tmat4x4<T>{{
        {T(1.0)-T(2.0)*v.y*v.y - T(2.0)*v.z*v.z,T(2.0)*v.x*v.y - T(2.0)*v.z*v.w,         T(2.0)*v.x*v.z + T(2.0)*v.y*v.w,         T(0.0)},
        {T(2.0)*v.x*v.y + T(2.0)*v.z*v.w,       T(1.0) - T(2.0)*v.x*v.x - T(2.0)*v.z*v.z,T(2.0)*v.y*v.z - T(2.0)*v.x*v.w,         T(0.0)},
        {T(2.0)*v.x*v.z - T(2.0)*v.y*v.w,       T(2.0)*v.y*v.z + T(2.0)*v.x*v.w,         T(1.0) - T(2.0)*v.x*v.x - T(2.0)*v.y*v.y,T(0.0)},
        {T(0.0),                                T(0.0),                                  T(0.0),                                  T(1.0)}
    }};
}

template<class T> inline tmat4x4<T> translate(const tvec3<T>& v)
{
    return tmat4x4<T>{{
        {T(1.0), T(0.0), T(0.0), T(0.0)},
        {T(0.0), T(1.0), T(0.0), T(0.0)},
        {T(0.0), T(0.0), T(1.0), T(0.0)},
        {   v.x,    v.y,    v.z, T(1.0)}
    }};
}

template<class T> inline tmat4x4<T> scale44(const tvec3<T>& v)
{
    return tmat4x4<T>{{
        {   v.x, T(0.0), T(0.0), T(0.0)},
        {T(0.0),    v.y, T(0.0), T(0.0)},
        {T(0.0), T(0.0),    v.z, T(0.0)},
        {T(0.0), T(0.0), T(0.0), T(1.0)},
    }};
}

template<class T> inline tmat4x4<T> transform(const tvec3<T>& t, const tquat<T>& r, const tvec3<T>& s)
{
    auto ret = scale44(s);
    ret *= to_mat4x4(r);
    (tvec3<T>&)ret[3] = t;
    return ret;
}

template<class T> inline tmat4x4<T> transpose(const tmat4x4<T>& v)
{
    return tmat4x4<T>{{
        {v[0][0], v[1][0], v[2][0], v[3][0]},
        {v[0][1], v[1][1], v[2][1], v[3][1]},
        {v[0][2], v[1][2], v[2][2], v[3][2]},
        {v[0][3], v[1][3], v[2][3], v[3][3]},
    }};
}

template<class T> inline tmat4x4<T> operator*(const tmat4x4<T>& a, const tmat4x4<T>& b_)
{
    const auto b = transpose(b_);
    return tmat4x4<T>{{
        { sum(a[0] * b[0]), sum(a[0] * b[1]), sum(a[0] * b[2]), sum(a[0] * b[3]) },
        { sum(a[1] * b[0]), sum(a[1] * b[1]), sum(a[1] * b[2]), sum(a[1] * b[3]) },
        { sum(a[2] * b[0]), sum(a[2] * b[1]), sum(a[2] * b[2]), sum(a[2] * b[3]) },
        { sum(a[3] * b[0]), sum(a[3] * b[1]), sum(a[3] * b[2]), sum(a[3] * b[3]) },
    }};
}
template<class T> inline tmat4x4<T>& operator*=(tmat4x4<T>& a, const tmat4x4<T>& b)
{
    a = a * b;
    return a;
}

} // namespace sfbx

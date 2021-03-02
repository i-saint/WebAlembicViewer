#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <type_traits>
#ifdef __cpp_lib_span
    #include <span>
#endif

#define sfbxEnableIf(...) std::enable_if_t<__VA_ARGS__, bool> = true

namespace sfbx {
    
#ifdef __cpp_lib_span

template<class T> using span = std::span<T>;

#else

// equivalent of std::span in C++20 (with dynamic_extent)
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
    template<class Cont>
    span(const Cont& v) : m_data(const_cast<T*>(v.data())), m_size(v.size()) {}
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

template<class T>
inline span<T> make_span(const T* v, size_t n) { return { const_cast<T*>(v), n }; }

// Container must have data(), size() and value_type. mainly intended to std::vector and sfbx::RawVector.
template<class Cont>
inline span<typename Cont::value_type> make_span(const Cont& v) { return { const_cast<typename Cont::value_type*>(v.data()), v.size() }; }


class noncopyable
{
protected:
    noncopyable() = default;
    ~noncopyable() = default;
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable) = delete;
};

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

    T* data() { return (T*)this; }
    const T* data() const { return (T*)this; }
    T& operator[](int i) { return data()[i]; }
    const T& operator[](int i) const { return data()[i]; }
    bool operator==(const tvec2& v) const { return x == v.x && y == v.y; }
    bool operator!=(const tvec2& v) const { return !((*this) == v); }
    template<class U> operator tvec2<U>() const { return { (U)x, (U)y }; }

    static constexpr tvec2 zero() { return { (T)0, (T)0 }; }
    static constexpr tvec2 one()  { return { (T)1, (T)1 }; }
};

template<class T>
struct tvec3
{
    T x, y, z;

    T* data() { return (T*)this; }
    const T* data() const { return (T*)this; }
    T& operator[](int i) { return data()[i]; }
    const T& operator[](int i) const { return data()[i]; }
    bool operator==(const tvec3& v) const { return x == v.x && y == v.y && z == v.z; }
    bool operator!=(const tvec3& v) const { return !((*this) == v); }
    template<class U> operator tvec3<U>() const { return { (U)x, (U)y, (U)z }; }

    static constexpr tvec3 zero() { return { (T)0, (T)0, (T)0 }; }
    static constexpr tvec3 one()  { return { (T)1, (T)1, (T)1 }; }
};

template<class T>
struct tvec4
{
    T x, y, z, w;

    T* data() { return (T*)this; }
    const T* data() const { return (T*)this; }
    T& operator[](int i) { return data()[i]; }
    const T& operator[](int i) const { return data()[i]; }
    bool operator==(const tvec4& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    bool operator!=(const tvec4& v) const { return !((*this) == v); }
    template<class U> operator tvec4<U>() const { return { (U)x, (U)y, (U)z, (U)w }; }

    static constexpr tvec4 zero() { return { (T)0, (T)0, (T)0, (T)0 }; }
    static constexpr tvec4 one()  { return { (T)1, (T)1, (T)1, (T)1 }; }
};

template<class T>
struct tquat
{
    T x, y, z, w;

    T* data() { return (T*)this; }
    const T* data() const { return (T*)this; }
    T& operator[](int i) { return data()[i]; }
    const T& operator[](int i) const { return data()[i]; }
    bool operator==(const tquat& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    bool operator!=(const tquat& v) const { return !((*this) == v); }
    template<class U> operator tquat<U>() const { return { (U)x, (U)y, (U)z, (U)w }; }

    static constexpr tquat identity() { return{ (T)0, (T)0, (T)0, (T)1 }; }
};

template<class T>
struct tmat4x4
{
    tvec4<T> m[4];

    T* data() { return (T*)this; }
    const T* data() const { return (T*)this; }
    tvec4<T>& operator[](int i) { return m[i]; }
    const tvec4<T>& operator[](int i) const { return m[i]; }
    bool operator==(const tmat4x4& v) const { return memcmp(m, v.m, sizeof(*this)) == 0; }
    bool operator!=(const tmat4x4& v) const { return !((*this) == v); }

    template<class U> operator tmat4x4<U>() const
    {
        return { { tvec4<U>(m[0]), tvec4<U>(m[1]), tvec4<U>(m[2]), tvec4<U>(m[3]) } };
    }

    template<class U> void assign(const U* s) const
    {
        T* d = (T*)this;
        for (size_t i = 0; i < 16; ++i)
            *d++ = T(*s++);
    }

    static constexpr tmat4x4 identity()
    {
        return{ {
            { (T)1, (T)0, (T)0, (T)0 },
            { (T)0, (T)1, (T)0, (T)0 },
            { (T)0, (T)0, (T)1, (T)0 },
            { (T)0, (T)0, (T)0, (T)1 }
        } };
    }
};

enum class RotationOrder : int
{
    XYZ,
    XZY,
    YZX,
    YXZ,
    ZXY,
    ZYX,
    SphericXYZ
};

using int2 = tvec2<int>;
using int3 = tvec3<int>;
using int4 = tvec4<int>;

using float2 = tvec2<float>;
using float3 = tvec3<float>;
using float4 = tvec4<float>;
using quatf = tquat<float>;
using float4x4 = tmat4x4<float>;

using double2 = tvec2<double>;
using double3 = tvec3<double>;
using double4 = tvec4<double>;
using quatd = tquat<double>;
using double4x4 = tmat4x4<double>;


#define sfbxEachObjectType(Body)\
    Body(NodeAttribute)\
    Body(Model) Body(Light) Body(Camera) Body(Root) Body(LimbNode)\
    Body(Geometry) Body(Mesh) Body(Shape)\
    Body(Deformer) Body(Skin) Body(Cluster) Body(BlendShape) Body(BlendShapeChannel)\
    Body(Pose) Body(BindPose)\
    Body(Material)\
    Body(AnimationStack) Body(AnimationLayer) Body(AnimationCurveNode) Body(AnimationCurve)

#define Decl(T) class T; using T##Ptr = std::shared_ptr<T>;
Decl(Property)
Decl(Node)

Decl(Object)
sfbxEachObjectType(Decl)

Decl(Document)
#undef Decl

template<class T, class U>
T* as(U* v) { return dynamic_cast<T*>(v); }

} // namespace sfbx

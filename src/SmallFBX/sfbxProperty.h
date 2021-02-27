#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "sfbxTypes.h"
#include "sfbxRawVector.h"

namespace sfbx {

template<class T> struct is_propery_pod { static constexpr bool value = false; };
#define PropPOD(T) template<> struct is_propery_pod<T> { static constexpr bool value = true; }
PropPOD(bool);
PropPOD(int8);
PropPOD(int16);
PropPOD(int32);
PropPOD(int64);
PropPOD(float32);
PropPOD(float64);
#undef PropPOD

template<class T> struct is_propery_array { static constexpr bool value = is_propery_pod<T>::value; };
#define PropPOD(T) template<> struct is_propery_array<T> { static constexpr bool value = true; }
PropPOD(float2);
PropPOD(float3);
PropPOD(float4);
PropPOD(double2);
PropPOD(double3);
PropPOD(double4);
#undef PropPOD


enum class PropertyType : uint8_t
{
    Bool    = 'B', // bool
    Int8    = 'C', // int8
    Int16   = 'Y', // int16
    Int32   = 'I', // int32
    Int64   = 'L', // int64
    Float32 = 'F', // float32
    Float64 = 'D', // float64

    String  = 'S', // std::string
    Blob    = 'R', // span<char>

    BoolArray    = 'b', // span<bool>
    Int8Array    = 'c', // span<int8>
    Int16Array   = 'y', // span<int16>
    Int32Array   = 'i', // span<int32>
    Int64Array   = 'l', // span<int64>
    Float32Array = 'f', // span<float32>
    Float64Array = 'd', // span<float64>
};
uint32_t SizeOfElement(PropertyType type);

class Property
{
friend class Document;
private:
    Property();

public:
    void read(std::istream& input);
    void write(std::ostream& output);

    template<class T> span<T> allocateArray(size_t size);

    // T: corresponding types with PropertyType (bool ... float64 and span<> & std::vector<>, std::string)
    template<class T, sfbxEnableIf(is_propery_pod<T>::value)> void assign(T v);
    template<class T, sfbxEnableIf(is_propery_array<T>::value)> void assign(span<T> v);
    template<class T> void assign(const std::vector<T>& v) { assign(make_span(v)); }
    template<class T> void assign(const RawVector<T>& v) { assign(make_span(v)); }
    // std::vector<bool> needs special care because it is actually a bit field.
    void assign(const std::vector<bool>& v);
    void assign(const std::string& v);
    void assign(const char* v);
    void assign(PropertyType t, const RawVector<char>& v);

    uint64_t getSizeInBytes() const;
    PropertyType getType() const;
    bool isArray() const;
    uint64_t getArraySize() const;

    template<class T> T getValue() const;
    template<class T> span<T> getArray() const;
    std::string getString() const;

    std::string toString() const;

private:
    PropertyType m_type{};
    union {
        int8 i8;
        int16 i16;
        int32 i32;
        int64 i64;
        float32 f32;
        float64 f64;
    } m_scalar{};
    RawVector<char> m_data;
};

} // namespace sfbx

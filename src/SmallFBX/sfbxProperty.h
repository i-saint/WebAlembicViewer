#pragma once

#include "sfbxTypes.h"
#include "sfbxRawVector.h"

namespace sfbx {

template<class T> struct is_propery_pod : std::false_type {};
#define PropPOD(T) template<> struct is_propery_pod<T> : std::true_type {}
PropPOD(bool);
PropPOD(boolean);
PropPOD(int16);
PropPOD(int32);
PropPOD(int64);
PropPOD(float32);
PropPOD(float64);

PropPOD(float2);
PropPOD(float3);
PropPOD(float4);
PropPOD(float4x4);
PropPOD(double2);
PropPOD(double3);
PropPOD(double4);
PropPOD(double4x4);
#undef PropPOD


enum class PropertyType : uint8_t
{
    Bool    = 'C', // boolean (not built-in bool. see struct boolean in sfbxTypes.h)
    Int16   = 'Y', // int16
    Int32   = 'I', // int32
    Int64   = 'L', // int64
    Float32 = 'F', // float32
    Float64 = 'D', // float64

    String  = 'S', // std::string
    Blob    = 'R', // RawVector<char>

    BoolArray    = 'b', // span<boolean>
    Int16Array   = 'y', // span<int16>
    Int32Array   = 'i', // span<int32>
    Int64Array   = 'l', // span<int64>
    Float32Array = 'f', // span<float32>
    Float64Array = 'd', // span<float64>
};
uint32_t SizeOfElement(PropertyType type);

class Property
{
public:
    Property();
    void read(std::istream& input);
    void write(std::ostream& output);

    template<class T> span<T> allocateArray(size_t size);

    // T: corresponding types with PropertyType (boolean ... float64 and span<> & std::vector<>, std::string)
    template<class T, sfbxEnableIf(is_propery_pod<T>::value)> void assign(T v);
    template<class T> void assign(span<T> v);
    template<class T> void assign(const std::vector<T>& v) { assign(make_span(v)); }
    template<class T> void assign(const RawVector<T>& v) { assign(make_span(v)); }
    void assign(const std::string& v);
    void assign(const char* v);

    PropertyType getType() const;
    bool isArray() const;
    uint64_t getArraySize() const;

    template<class T> T getValue() const;
    template<class T> span<T> getArray() const;
    std::string getString() const;

    std::string toString(int depth = 0) const;

private:
    PropertyType m_type{};
    union {
        boolean b;
        int16 i16;
        int32 i32;
        int64 i64;
        float32 f32;
        float64 f64;
    } m_scalar{};
    RawVector<char> m_data;
};

} // namespace sfbx

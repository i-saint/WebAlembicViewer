#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "sfbxTypes.h"

namespace sfbx {

enum class PropertyType : uint8_t
{
    Bool = 'B',     // bool
    Int8 = 'C',     // int8
    Int16 = 'Y',    // int16
    Int32 = 'I',    // int32
    Int64 = 'L',    // int64
    Float32 = 'F',  // float32
    Float64 = 'D',  // float64

    String = 'S',   // std::string
    Raw = 'R',      // span<char>

    BoolArray = 'b',    // span<bool>
    Int8Array = 'c',    // span<int8>
    Int16Array = 'y',   // span<int16>
    Int32Array = 'i',   // span<int32>
    Int64Array = 'l',   // span<int64>
    Float32Array = 'f', // span<float32>
    Float64Array = 'd', // span<float64>
};

class Property
{
public:
    explicit Property(std::istream &is);
    explicit Property(const char* v);
    template<class T> explicit Property(const T& v);
    Property(PropertyType type, const std::vector<char>& data);

    template<class T> void operator=(const T& v);
    void operator=(const char* v);

    void read(std::istream& input);
    void write(std::ostream& output);

    PropertyType getType() const;
    bool isArray() const;
    uint32_t getArraySize() const;
    uint32_t getSizeInBytes() const;

    template<class T> T getValue() const;
    template<class T> span<T> getArray() const;
    std::string getString() const;

    std::string toString() const;

private:
    PropertyType m_type;
    union {
        bool b;
        int8 i8;
        int16 i16;
        int32 i32;
        int64 i64;
        float32 f32;
        float64 f64;
    } m_scalar{};
    std::vector<char> m_data;
};

template<class... T>
inline PropertyPtr MakeProperty(T&&... v)
{
    return std::make_shared<Property>(std::forward<T>(v)...);
}

} // namespace sfbx

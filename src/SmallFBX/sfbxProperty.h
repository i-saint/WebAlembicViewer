#pragma once

#include <iostream>
#include <vector>

namespace sfbx {

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using float32 = float;
using float64 = double;

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
    Raw = 'R',      // std::span<char>

    BoolArray = 'b',    // std::span<bool>
    Int8Array = 'c',    // std::span<int8>
    Int16Array = 'y',   // std::span<int16>
    Int32Array = 'i',   // std::span<int32>
    Int64Array = 'l',   // std::span<int64>
    Float32Array = 'f', // std::span<float32>
    Float64Array = 'd', // std::span<float64>
};

class Property
{
public:
    explicit Property(std::istream &is);
    explicit Property(const char* v);
    template<class T> explicit Property(const T& v);
    Property(PropertyType type, const std::vector<char>& data);

    void read(std::istream& input);
    void write(std::ostream& output);

    template<class T> void operator=(const T& v);
    void operator=(const char* v);

    std::string toString() const;
    PropertyType getType() const;
    bool isArray() const;
    uint32_t getArraySize() const;
    uint32_t getBytes() const;

    template<class T> T getValue() const;
    template<class T> std::span<T> getArray() const;
    std::string getString() const;

private:
    PropertyType m_type;
    std::vector<char> m_data;
};

} // namespace sfbx

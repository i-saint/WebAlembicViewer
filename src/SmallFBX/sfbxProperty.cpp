#include "pch.h"
#include "sfbxProperty.h"
#include "sfbxUtil.h"

#include <zlib.h>
#pragma comment(lib, "zlib.lib")


namespace sfbx {

uint32_t SizeOfElement(PropertyType type)
{
    switch (type) {
    case PropertyType::BoolArray:
    case PropertyType::Int8Array:
        return 1;

    case PropertyType::Int16Array:
        return 2;

    case PropertyType::Int32Array:
    case PropertyType::Float32Array:
        return 4;

    case PropertyType::Int64Array:
    case PropertyType::Float64Array:
        return 8;

    default:
        return 1;
    }
}

Property::Property() {}

template<class T>
static inline void Assign(RawVector<char>& dst, const span<T>& v)
{
    size_t s = sizeof(T) * v.size();
    dst.resize(s);
    dst.assign((char*)v.data(), (char*)v.data() + s);
}

template<> void Property::assign(bool v)    { m_type = PropertyType::Bool; m_scalar.i8 = v; }
template<> void Property::assign(int8 v)    { m_type = PropertyType::Int8; m_scalar.i8 = v; }
template<> void Property::assign(int16 v)   { m_type = PropertyType::Int16; m_scalar.i16 = v; }
template<> void Property::assign(int32 v)   { m_type = PropertyType::Int32; m_scalar.i32 = v; }
template<> void Property::assign(int64 v)   { m_type = PropertyType::Int64; m_scalar.i64 = v; }
template<> void Property::assign(float32 v) { m_type = PropertyType::Float32; m_scalar.f32 = v; }
template<> void Property::assign(float64 v) { m_type = PropertyType::Float64; m_scalar.f64 = v; }

template<> void Property::assign(span<bool> v)    { m_type = PropertyType::BoolArray; Assign(m_data, v); }
template<> void Property::assign(span<int8> v)    { m_type = PropertyType::Int8Array; Assign(m_data, v); }
template<> void Property::assign(span<int16> v)   { m_type = PropertyType::Int16Array; Assign(m_data, v); }
template<> void Property::assign(span<int32> v)   { m_type = PropertyType::Int32Array; Assign(m_data, v); }
template<> void Property::assign(span<int64> v)   { m_type = PropertyType::Int64Array; Assign(m_data, v); }
template<> void Property::assign(span<float32> v) { m_type = PropertyType::Float32Array; Assign(m_data, v); }
template<> void Property::assign(span<float64> v) { m_type = PropertyType::Float64Array; Assign(m_data, v); }

template<> void Property::assign(span<float2> v)  { assign(span<float32>{ (float32*)v.data(), v.size() * 2 }); }
template<> void Property::assign(span<float3> v)  { assign(span<float32>{ (float32*)v.data(), v.size() * 3 }); }
template<> void Property::assign(span<float4> v)  { assign(span<float32>{ (float32*)v.data(), v.size() * 4 }); }
template<> void Property::assign(span<double2> v) { assign(span<float64>{ (float64*)v.data(), v.size() * 2 }); }
template<> void Property::assign(span<double3> v) { assign(span<float64>{ (float64*)v.data(), v.size() * 3 }); }
template<> void Property::assign(span<double4> v) { assign(span<float64>{ (float64*)v.data(), v.size() * 4 }); }

void Property::assign(const std::vector<bool>& v)
{
    m_type = PropertyType::BoolArray;
    size_t s = v.size();
    m_data.resize(s);
    for (size_t i = 0; i < s; ++i)
        m_data[i] = (int8)v[i];
}

void Property::assign(const std::string& v)
{
    m_type = PropertyType::String;
    m_data.assign(v.begin(), v.end());
}

void Property::assign(const char* v)
{
    m_type = PropertyType::String;
    m_data.clear();
    if (v)
        m_data.assign(v, v + std::strlen(v));
}

void Property::assign(PropertyType t, const RawVector<char>& v)
{
    m_type = t;
    m_data = v;
}

void Property::read(std::istream& is)
{
    m_type = read1<PropertyType>(is);
    if (m_type == PropertyType::String || m_type == PropertyType::Blob) {
        uint32_t length = read1<uint32_t>(is);
        m_data.resize(length);
        is.read(m_data.data(), length);
    }
    else if (!isArray()) {
        if (m_type == PropertyType::Bool || m_type == PropertyType::Int8)
            m_scalar.i8 = read1<int8>(is);
        else if (m_type == PropertyType::Int16)
            m_scalar.i16 = read1<int16>(is);
        else if (m_type == PropertyType::Int32 || m_type == PropertyType::Float32)
            m_scalar.i32 = read1<int32>(is);
        else if (m_type == PropertyType::Int64 || m_type == PropertyType::Float64)
            m_scalar.i64 = read1<int64>(is);
        else
            throw std::runtime_error(std::string("Unsupported property type ") + std::to_string((char)m_type));
    }
    else {
        uint32_t array_size = read1<uint32_t>(is); // number of elements in array
        uint32_t encoding = read1<uint32_t>(is); // 0 .. uncompressed, 1 .. zlib-compressed

        uLong src_size = read1<uint32_t>(is);
        uLong dest_size = SizeOfElement(m_type) * array_size;
        m_data.resize(dest_size);

        if (encoding) {
            std::vector<char> compressed_buffer(src_size);
            readv(is, compressed_buffer.data(), src_size);
            uncompress2((Bytef*)m_data.data(), &dest_size, (const Bytef*)compressed_buffer.data(), &src_size);

            if (src_size != compressed_buffer.size())
                throw std::runtime_error("compressedLength does not match data");
            if (dest_size != m_data.size())
                throw std::runtime_error("uncompressedLength does not match data");
        }
        else {
            readv(is, m_data.data(), dest_size);
        }
    }
}

void Property::write(std::ostream& os)
{
    write1(os, m_type);
    if (m_type == PropertyType::Blob || m_type == PropertyType::String) {
        write1(os, (uint32_t)m_data.size());
        writev(os, m_data.data(), m_data.size());
    }
    else if (!isArray()) {
        // scalar
        writev(os, m_data.data(), m_data.size());
    }
    else {
        // array
        write1(os, (uint32_t)getArraySize()); // arrayLength
        write1(os, (uint32_t)0); // encoding // TODO: support compression
        write1(os, (uint32_t)m_data.size());
    }
}

static inline char Base16Letter(uint8_t n)
{
    n %= 16;
    if (n <= 9)
        return n + '0';
    return n + 'a' - 10;
}

static inline std::string Base16Number(uint8_t n)
{
    return std::string() + Base16Letter(n >> 4) + Base16Letter(n);
}

uint32_t Property::getSizeInBytes() const
{
    switch (m_type) {
    case PropertyType::Bool:
    case PropertyType::Int8:
        return 1 + 1;

    case PropertyType::Int16:
        return 2 + 1;

    case PropertyType::Int32:
    case PropertyType::Float32:
        return 4 + 1;

    case PropertyType::Int64:
    case PropertyType::Float64:
        return 4 + 1;

    case PropertyType::String:
    case PropertyType::Blob:
        return m_data.size() + 4 + 1;

    default:
        return m_data.size() + 12 + 1;
    }
}

PropertyType Property::getType() const
{
    return m_type;
}

bool Property::isArray() const
{
    return (char)m_type > 'Z';
}

uint32_t Property::getArraySize() const
{
    return m_data.size() / SizeOfElement(m_type);
}

#define ReturnIfNull() if (!this) { return {}; }

template<> bool    Property::getValue() const { ReturnIfNull(); return m_scalar.i8; }
template<> int8    Property::getValue() const { ReturnIfNull(); return m_scalar.i8; }
template<> int16   Property::getValue() const { ReturnIfNull(); return m_scalar.i16; }
template<> int32   Property::getValue() const { ReturnIfNull(); return m_scalar.i32; }
template<> int64   Property::getValue() const { ReturnIfNull(); return m_scalar.i64; }
template<> float32 Property::getValue() const { ReturnIfNull(); return m_scalar.f32; }
template<> float64 Property::getValue() const { ReturnIfNull(); return m_scalar.f64; }

template<> double4x4 Property::getValue() const
{
    ReturnIfNull();
    double4x4 r;
    r.assign((float64*)m_data.data());
    return r;
}

template<> span<bool>    Property::getArray() const { ReturnIfNull(); return make_span((bool*)m_data.data(), getArraySize()); }
template<> span<int8>    Property::getArray() const { ReturnIfNull(); return make_span((int8*)m_data.data(), getArraySize()); }
template<> span<int16>   Property::getArray() const { ReturnIfNull(); return make_span((int16*)m_data.data(), getArraySize()); }
template<> span<int32>   Property::getArray() const { ReturnIfNull(); return make_span((int32*)m_data.data(), getArraySize()); }
template<> span<int64>   Property::getArray() const { ReturnIfNull(); return make_span((int64*)m_data.data(), getArraySize()); }
template<> span<float32> Property::getArray() const { ReturnIfNull(); return make_span((float32*)m_data.data(), getArraySize()); }
template<> span<float64> Property::getArray() const { ReturnIfNull(); return make_span((float64*)m_data.data(), getArraySize()); }

template<> span<float2>  Property::getArray() const { ReturnIfNull(); return make_span((float2*)m_data.data(), getArraySize() / 2); }
template<> span<float3>  Property::getArray() const { ReturnIfNull(); return make_span((float3*)m_data.data(), getArraySize() / 3); }
template<> span<float4>  Property::getArray() const { ReturnIfNull(); return make_span((float4*)m_data.data(), getArraySize() / 4); }
template<> span<double2> Property::getArray() const { ReturnIfNull(); return make_span((double2*)m_data.data(), getArraySize() / 2); }
template<> span<double3> Property::getArray() const { ReturnIfNull(); return make_span((double3*)m_data.data(), getArraySize() / 3); }
template<> span<double4> Property::getArray() const { ReturnIfNull(); return make_span((double4*)m_data.data(), getArraySize() / 4); }

std::string Property::getString() const
{
    ReturnIfNull();
    return std::string(m_data.data(), m_data.size());
}

std::string Property::toString() const
{
    ReturnIfNull();
    if (isArray()) {
        auto toS = [](const auto& span) {
            std::string s;
            s += "[";
            bool first = true;
            for (auto v : span) {
                if (!first)
                    s += ", ";
                s += std::to_string(v);
                first = false;
            }
            s += "]";
            return s;
        };
        switch (m_type) {
        case PropertyType::BoolArray: return toS(getArray<bool>());
        case PropertyType::Int8Array: return toS(getArray<int8>());
        case PropertyType::Int16Array: return toS(getArray<int16>());
        case PropertyType::Int32Array: return toS(getArray<int32>());
        case PropertyType::Int64Array: return toS(getArray<int64>());
        case PropertyType::Float32Array: return toS(getArray<float32>());
        case PropertyType::Float64Array: return toS(getArray<float64>());
        default: break;
        }
    }
    else {
        switch (m_type) {
        case PropertyType::Bool: return getValue<bool>() ? "true" : "false";
        case PropertyType::Int8: return std::to_string(getValue<int8>());
        case PropertyType::Int16: return std::to_string(getValue<int16>());
        case PropertyType::Int32: return std::to_string(getValue<int32>());
        case PropertyType::Int64: return std::to_string(getValue<int64>());
        case PropertyType::Float32: return std::to_string(getValue<float32>());
        case PropertyType::Float64: return std::to_string(getValue<float64>());

        case PropertyType::Blob:
        {
            std::string s;
            s += "\"";
            for (char c : m_data)
                s += std::to_string(uint8_t(c)) + " ";
            s += "\"";
            return s;
        }
        case PropertyType::String:
        {
            std::string s;
            s += "\"";
            for (char c : m_data) {
                if (c == '\\') {
                    s += "\\\\";
                }
                else if (c >= 32 && c <= 126) {
                    s += c;
                }
                else {
                    s += "\\u00";
                    s += Base16Number(c);
                }
            }
            s += "\"";
            return s;
        }

        default:
            break;
        }
    }
    return "";
}

} // namespace sfbx

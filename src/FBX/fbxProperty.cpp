#include "pch.h"
#include "fbxProperty.h"
#include "fbxUtil.h"

#include <zlib.h>
#pragma comment(lib, "zlib.lib")

namespace fbx {

static uint32_t GetArrayElementSize(PropertyType type)
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
        return 0;
    }
}

template<class T>
static inline void Assign(std::vector<char>& dst, const T& v)
{
    size_t s = sizeof(T);
    dst.resize(s);
    dst.assign((char*)&v, (char*)&v + s);
}
template<class T>
static inline void Assign(std::vector<char>& dst, const std::vector<T>& v)
{
    size_t s = sizeof(T) * v.size();
    dst.resize(s);
    dst.assign((char*)v.data(), (char*)v.data() + s);
}

template<> void FBXProperty::operator=(const bool& v)    { m_type = PropertyType::Bool; Assign(m_data, (uint8_t)v); }
template<> void FBXProperty::operator=(const int16_t& v) { m_type = PropertyType::Int16; Assign(m_data, v); }
template<> void FBXProperty::operator=(const int32_t& v) { m_type = PropertyType::Int32; Assign(m_data, v); }
template<> void FBXProperty::operator=(const int64_t& v) { m_type = PropertyType::Int64; Assign(m_data, v); }
template<> void FBXProperty::operator=(const float& v)   { m_type = PropertyType::Float32; Assign(m_data, v); }
template<> void FBXProperty::operator=(const double& v)  { m_type = PropertyType::Float64; Assign(m_data, v); }

template<> void FBXProperty::operator=(const std::vector<int32_t>& v) { m_type = PropertyType::Int32Array; Assign(m_data, v); }
template<> void FBXProperty::operator=(const std::vector<int64_t>& v) { m_type = PropertyType::Int64Array; Assign(m_data, v); }
template<> void FBXProperty::operator=(const std::vector<float>& v)   { m_type = PropertyType::Float32Array; Assign(m_data, v); }
template<> void FBXProperty::operator=(const std::vector<double>& v)  { m_type = PropertyType::Float32Array; Assign(m_data, v); }

template<> void FBXProperty::operator=(const std::vector<bool>& v)
{
    // std::vector<bool> needs special care because it is actually a bit field.
    m_type = PropertyType::BoolArray;
    size_t s = v.size();
    m_data.resize(s);
    for (size_t i = 0; i < s; ++i)
        m_data[i] = (char)v[i];
}

template<> void FBXProperty::operator=(const std::string& v)
{
    m_type = PropertyType::String;
    m_data.assign(v.begin(), v.end());
}
void FBXProperty::operator=(const char* v)
{
    m_type = PropertyType::String;
    m_data.clear();
    if (v)
        m_data.assign(v, v + std::strlen(v));
}


template<class T> FBXProperty::FBXProperty(const T& v) { *this = v; }
template FBXProperty::FBXProperty(const bool& v);
template FBXProperty::FBXProperty(const int16_t& v);
template FBXProperty::FBXProperty(const int32_t& v);
template FBXProperty::FBXProperty(const int64_t& v);
template FBXProperty::FBXProperty(const float& v);
template FBXProperty::FBXProperty(const double& v);
template FBXProperty::FBXProperty(const std::vector<int32_t>& v);
template FBXProperty::FBXProperty(const std::vector<int64_t>& v);
template FBXProperty::FBXProperty(const std::vector<float>& v);
template FBXProperty::FBXProperty(const std::vector<double>& v);
template FBXProperty::FBXProperty(const std::string& v);
FBXProperty::FBXProperty(const char* v) { *this = v; }

FBXProperty::FBXProperty(PropertyType type, const std::vector<char>& data)
    : m_type(type)
    , m_data(data)
{}

FBXProperty::FBXProperty(std::istream& is)
{
    read(is);
}

void FBXProperty::read(std::istream& is)
{
    m_type = read1<PropertyType>(is);
    if (m_type == PropertyType::String || m_type == PropertyType::Raw) {
        uint32_t length = read1<uint32_t>(is);
        m_data.resize(length);
        is.read(m_data.data(), length);
    }
    else if ((char)m_type <= 'Z') {
        auto do_read = [&](size_t s) {
            m_data.resize(s);
            is.read(m_data.data(), s);
        };

        if (m_type == PropertyType::Bool || m_type == PropertyType::Int8)
            do_read(sizeof(uint8_t));
        else if (m_type == PropertyType::Int16)
            do_read(sizeof(int16_t));
        else if (m_type == PropertyType::Int32 || m_type == PropertyType::Float32)
            do_read(sizeof(int32_t));
        else if (m_type == PropertyType::Int64 || m_type == PropertyType::Float64)
            do_read(sizeof(int64_t));
        else
            throw std::runtime_error(std::string("Unsupported property type ") + std::to_string((char)m_type));
    }
    else {
        uint32_t array_size = read1<uint32_t>(is); // number of elements in array
        uint32_t encoding = read1<uint32_t>(is); // 0 .. uncompressed, 1 .. zlib-compressed

        uLong src_size = read1<uint32_t>(is);
        uLong dest_size = GetArrayElementSize(m_type) * array_size;
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

void FBXProperty::write(std::ostream& os)
{
    write1(os, m_type);
    if (m_type == PropertyType::Raw || m_type == PropertyType::String) {
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

PropertyType FBXProperty::getType() const
{
    return m_type;
}

bool FBXProperty::isArray() const
{
    return (char)m_type > 'Z';
}

uint32_t FBXProperty::getArraySize() const
{
    return isArray() ? m_data.size() / GetArrayElementSize(m_type) : 1;
}

template<class T> T FBXProperty::getValue() const
{
    return *(T*)m_data.data();
}

template<class T> std::span<T> FBXProperty::getArray() const
{
    return std::span<T>{ (T*)m_data.data(), m_data.size() / GetArrayElementSize(m_type) };
}

std::string FBXProperty::getString() const
{
    return std::string(m_data.data(), m_data.size());
}

std::string FBXProperty::toString() const
{
    switch (m_type) {
    case PropertyType::Bool: return getValue<bool>() ? "true" : "false";
    case PropertyType::Int8: return std::to_string(getValue<int8>());
    case PropertyType::Int16: return std::to_string(getValue<int16>());
    case PropertyType::Int32: return std::to_string(getValue<int32>());
    case PropertyType::Int64: return std::to_string(getValue<int64>());
    case PropertyType::Float32: return std::to_string(getValue<float32>());
    case PropertyType::Float64: return std::to_string(getValue<float64>());

    case PropertyType::Raw:
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
    return "";
}

uint32_t FBXProperty::getBytes() const
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
    case PropertyType::Raw:
        return m_data.size() + 4 + 1;

    default:
        return m_data.size() + 12 + 1;
    }
}

} // namespace fbx

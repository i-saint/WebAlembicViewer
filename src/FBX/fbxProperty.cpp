#include "pch.h"
#include "fbxProperty.h"
#include "fbxUtil.h"

#include <zlib.h>
#pragma comment(lib, "zlib.lib")

namespace fbx {

static uint32_t GetArrayElementSize(char type)
{
    type -= ('a' - 'A');

    if (type == 'C' || type == 'B') return 1; // 1 bit boolean (1: true, 0: false) encoded as the LSB of a 1 Byte value.
    else if (type == 'Y') return 2; // 2 byte signed integer
    else if (type == 'I') return 4; // 4 byte signed Integer
    else if (type == 'F') return 4; // 4 byte single-precision IEEE 754 number
    else if (type == 'D') return 8; // 8 byte double-precision IEEE 754 number
    else if (type == 'L') return 8; // 8 byte signed Integer
    return 0;
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

template<> void FBXProperty::operator=(const bool& v) { m_type = 'C'; Assign(m_data, (uint8_t)v); }
template<> void FBXProperty::operator=(const int16_t& v) { m_type = 'Y'; Assign(m_data, v); }
template<> void FBXProperty::operator=(const int32_t& v) { m_type = 'I'; Assign(m_data, v); }
template<> void FBXProperty::operator=(const int64_t& v) { m_type = 'L'; Assign(m_data, v); }
template<> void FBXProperty::operator=(const float& v) { m_type = 'F'; Assign(m_data, v); }
template<> void FBXProperty::operator=(const double& v) { m_type = 'D'; Assign(m_data, v); }

template<> void FBXProperty::operator=(const std::vector<int32_t>& v) { m_type = 'i'; Assign(m_data, v); }
template<> void FBXProperty::operator=(const std::vector<int64_t>& v) { m_type = 'l'; Assign(m_data, v); }
template<> void FBXProperty::operator=(const std::vector<float>& v) { m_type = 'f'; Assign(m_data, v); }
template<> void FBXProperty::operator=(const std::vector<double>& v) { m_type = 'd'; Assign(m_data, v); }

template<> void FBXProperty::operator=(const std::vector<bool>& v)
{
    // std::vector<bool> needs special care because it is actually a bit field.
    m_type = 'b';
    size_t s = v.size();
    m_data.resize(s);
    for (size_t i = 0; i < s; ++i)
        m_data[i] = (char)v[i];
}

template<> void FBXProperty::operator=(const std::string& v)
{
    m_type = 'S';
    m_data.assign(v.begin(), v.end());
}
void FBXProperty::operator=(const char* v)
{
    m_type = 'S';
    m_data.clear();
    if (v)
        m_data.assign(v, v + std::strlen(v));
}


FBXProperty::FBXProperty(const std::vector<char>& a, uint8_t type)
    : m_data(a)
{
    if (type != 'R' && type != 'S') {
        throw std::runtime_error("Bad argument to FBXProperty constructor");
    }
    m_type = type;
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



FBXProperty::FBXProperty(std::istream& is)
{
    read(is);
}

void FBXProperty::read(std::istream& is)
{
    m_type = read1<uint8_t>(is);
    if (m_type == 'S' || m_type == 'R') {
        uint32_t length = read1<uint32_t>(is);
        m_data.resize(length);
        is.read(m_data.data(), length);
    }
    else if (m_type <= 'Z') {
        auto do_read = [&](size_t s) {
            m_data.resize(s);
            is.read(m_data.data(), s);
        };

        if (m_type == 'C' || m_type == 'B') { // 1 bit boolean (1: true, 0: false) encoded as the LSB of a 1 Byte value.
            do_read(sizeof(uint8_t));
        }
        else if (m_type == 'Y') { // 2 byte signed integer
            do_read(sizeof(int16_t));
        }
        else if (m_type == 'I') { // 4 byte signed Integer
            do_read(sizeof(int32_t));
        }
        else if (m_type == 'L') { // 8 byte signed Integer
            do_read(sizeof(int64_t));
        }
        else if (m_type == 'F') { // 4 byte single-precision IEEE 754 number
            do_read(sizeof(float));
        }
        else if (m_type == 'D') { // 8 byte double-precision IEEE 754 number
            do_read(sizeof(double));
        }
        else {
            throw std::runtime_error(std::string("Unsupported property type ") + std::to_string(m_type));
        }
    }
    else {
        uint32_t arrayLength = read1<uint32_t>(is); // number of elements in array
        uint32_t encoding = read1<uint32_t>(is); // 0 .. uncompressed, 1 .. zlib-compressed

        uLong srcLen = read1<uint32_t>(is);
        uLong destLen = GetArrayElementSize(m_type) * arrayLength;
        m_data.resize(destLen);

        if (encoding) {
            std::vector<char> compressedBuffer(srcLen);
            is.read(compressedBuffer.data(), srcLen);
            uncompress2((Bytef*)m_data.data(), &destLen, (const Bytef*)compressedBuffer.data(), &srcLen);

            if (srcLen != compressedBuffer.size())
                throw std::runtime_error("compressedLength does not match data");
            if (destLen != m_data.size())
                throw std::runtime_error("uncompressedLength does not match data");
        }
        else {
            is.read(m_data.data(), destLen);
        }
    }
}

void FBXProperty::write(std::ostream& os)
{
    os.write((char*)&m_type, sizeof(m_type));
    if (m_type == 'R' || m_type == 'S') {
        uint32_t size = m_data.size();
        os.write((char*)&size, sizeof(size));
        os.write(m_data.data(), m_data.size());
    }
    else if (m_type <= 'Z') {
        os.write(m_data.data(), m_data.size());
    }
    else {
        uint32_t array_size = getArraySize();
        uint32_t encoding = 0;
        uint32_t size = (uint32_t)m_data.size();

        os.write((char*)&array_size, sizeof(array_size)); // arrayLength
        os.write((char*)&encoding, sizeof(encoding)); // encoding // TODO: support compression
        os.write((char*)&size, sizeof(size));
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

char FBXProperty::getType() const
{
    return m_type;
}

bool FBXProperty::isArray() const
{
    return m_type >= 'a';
}

uint32_t FBXProperty::getArraySize() const
{
    if (m_type <= 'Z') {
        return 1;
    }
    else {
        return m_data.size() / GetArrayElementSize(m_type);
    }
}
template<class T> T FBXProperty::getScalar() const
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
    if (m_type == 'Y') return std::to_string(getScalar<int16_t>());
    else if (m_type == 'C') return getScalar<bool>() ? "true" : "false";
    else if (m_type == 'I') return std::to_string(getScalar<int32_t>());
    else if (m_type == 'L') return std::to_string(getScalar<int64_t>());
    else if (m_type == 'F') return std::to_string(getScalar<float>());
    else if (m_type == 'D') return std::to_string(getScalar<double>());
    else if (m_type == 'R') {
        std::string s;
        s += "\"";
        for(char c : m_data)
            s += std::to_string(uint8_t(c)) + " ";
        s += "\"";
        return s;
    }
    else if(m_type == 'S') {
        std::string s;
        s += "\"";
        for(char c : m_data) {
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
    else {
        std::string s;

        auto toS = [&s](const auto& span) {
            s += "[";
            bool hasPrev = false;
            for (auto v: span) {
                if (hasPrev) s += ", ";
                s += std::to_string(v);
                hasPrev = true;
            }
            s += "]";
        };

        if (m_type == 'b')      { toS(getArray<uint8_t>()); } // todo: bool
        else if (m_type == 'i') { toS(getArray<int32_t>()); }
        else if (m_type == 'l') { toS(getArray<int64_t>()); }
        else if (m_type == 'f') { toS(getArray<float>()); }
        else if (m_type == 'd') { toS(getArray<double>()); }

        return s;
    }
}

uint32_t FBXProperty::getBytes() const
{
    if (m_type == 'C') return 1 + 1;
    else if (m_type == 'Y') return 2 + 1; // 2 for int16, 1 for type spec
    else if (m_type == 'I') return 4 + 1;
    else if (m_type == 'F') return 4 + 1;
    else if (m_type == 'D') return 8 + 1;
    else if (m_type == 'L') return 8 + 1;
    else if (m_type == 'R') return m_data.size() + 4 + 1;
    else if (m_type == 'S') return m_data.size() + 4 + 1;
    else if (m_type == 'f') return m_data.size() + 12 + 1;
    else if (m_type == 'd') return m_data.size() + 12 + 1;
    else if (m_type == 'l') return m_data.size() + 12 + 1;
    else if (m_type == 'i') return m_data.size() + 12 + 1;
    else if (m_type == 'b') return m_data.size() + 12 + 1;
    else throw std::runtime_error("Invalid property");
}

} // namespace fbx

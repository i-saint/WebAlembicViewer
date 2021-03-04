#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxProperty.h"

#include <zlib.h>
#pragma comment(lib, "zlib.lib")


namespace sfbx {

uint32_t SizeOfElement(PropertyType type)
{
    switch (type) {
    case PropertyType::BoolArray:
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

void Property::read(std::istream& is)
{
    m_type = read1<PropertyType>(is);
    if (m_type == PropertyType::String || m_type == PropertyType::Blob) {
        uint32_t length = read1<uint32_t>(is);
        m_data.resize(length);
        is.read(m_data.data(), length);
    }
    else if (!isArray()) {
        switch (m_type) {
        case PropertyType::Bool:
            m_scalar.b = read1<boolean>(is);
            break;
        case PropertyType::Int16:
            m_scalar.i16 = read1<int16>(is);
            break;
        case PropertyType::Int32:
        case PropertyType::Float32:
            m_scalar.i32 = read1<int32>(is);
            break;
        case PropertyType::Int64:
        case PropertyType::Float64:
            m_scalar.i64 = read1<int64>(is);
            break;
        default:
            throw std::runtime_error(std::string("Unsupported property type ") + std::to_string((char)m_type));
            break;
        }
    }
    else {
        uint32_t array_size = read1<uint32_t>(is); // number of elements in array
        uint32_t encoding = read1<uint32_t>(is); // 0 .. uncompressed, 1 .. zlib-compressed

        uLong src_size = read1<uint32_t>(is);
        uLong dest_size = SizeOfElement(m_type) * array_size;
        m_data.resize(dest_size);

        if (encoding == 0) {
            readv(is, m_data.data(), dest_size);
        }
        else if (encoding == 1) {
            RawVector<char> compressed_buffer(src_size);
            readv(is, compressed_buffer.data(), src_size);
            uncompress2((Bytef*)m_data.data(), &dest_size, (const Bytef*)compressed_buffer.data(), &src_size);
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
        switch (m_type) {
        case PropertyType::Bool:
            write1(os, m_scalar.b);
            break;
        case PropertyType::Int16:
            write1(os, m_scalar.i16);
            break;
        case PropertyType::Int32:
        case PropertyType::Float32:
            write1(os, m_scalar.i32);
            break;
        case PropertyType::Int64:
        case PropertyType::Float64:
            write1(os, m_scalar.i64);
            break;
        default:
            printf("sfbx::Property::write(): Unsupported property type %c\n", (char)m_type);
            break;
        }
    }
    else {
        // array
        write1(os, (uint32_t)getArraySize()); // arrayLength
        write1(os, (uint32_t)0); // encoding // TODO: support compression
        write1(os, (uint32_t)m_data.size());
        writev(os, m_data.data(), m_data.size());
    }
}


template<> span<int32> Property::allocateArray(size_t size)
{
    m_type = PropertyType::Int32Array;
    m_data.resize(size * sizeof(int32));
    return make_span((int32*)m_data.data(), size);
}

template<> span<float64> Property::allocateArray(size_t size)
{
    m_type = PropertyType::Float64Array;
    m_data.resize(size * sizeof(float64));
    return make_span((float64*)m_data.data(), size);
}
template<> span<double2> Property::allocateArray(size_t size)
{
    m_type = PropertyType::Float64Array;
    m_data.resize(size * sizeof(double2));
    return make_span((double2*)m_data.data(), size);
}
template<> span<double3> Property::allocateArray(size_t size)
{
    m_type = PropertyType::Float64Array;
    m_data.resize(size * sizeof(double3));
    return make_span((double3*)m_data.data(), size);
}
template<> span<double4> Property::allocateArray(size_t size)
{
    m_type = PropertyType::Float64Array;
    m_data.resize(size * sizeof(double4));
    return make_span((double4*)m_data.data(), size);
}

template<class T>
static inline void Assign(RawVector<char>& dst, const span<T>& v)
{
    size_t s = sizeof(T) * v.size();
    dst.resize(s);
    dst.assign((char*)v.data(), (char*)v.data() + s);
}

template<> void Property::assign(boolean v) { m_type = PropertyType::Bool; m_scalar.b = v; }
template<> void Property::assign(bool v)    { m_type = PropertyType::Bool; m_scalar.b = v; }
template<> void Property::assign(int16 v)   { m_type = PropertyType::Int16; m_scalar.i16 = v; }
template<> void Property::assign(int32 v)   { m_type = PropertyType::Int32; m_scalar.i32 = v; }
template<> void Property::assign(int64 v)   { m_type = PropertyType::Int64; m_scalar.i64 = v; }
template<> void Property::assign(float32 v) { m_type = PropertyType::Float32; m_scalar.f32 = v; }
template<> void Property::assign(float64 v) { m_type = PropertyType::Float64; m_scalar.f64 = v; }

template<> void Property::assign(span<char> v)    { m_type = PropertyType::String; Assign(m_data, v); }
template<> void Property::assign(span<uint8_t> v) { m_type = PropertyType::Blob; Assign(m_data, v); }
template<> void Property::assign(span<boolean> v) { m_type = PropertyType::BoolArray; Assign(m_data, v); }
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

PropertyType Property::getType() const
{
    return m_type;
}

bool Property::isArray() const
{
    return (char)m_type > 'Z';
}

uint64_t Property::getArraySize() const
{
    return m_data.size() / SizeOfElement(m_type);
}

template<> boolean Property::getValue() const { return m_scalar.b; }
template<> bool    Property::getValue() const { return m_scalar.b; }
template<> int16   Property::getValue() const { return m_scalar.i16; }
template<> int32   Property::getValue() const { return m_scalar.i32; }
template<> int64   Property::getValue() const { return m_scalar.i64; }
template<> float32 Property::getValue() const { return m_scalar.f32; }
template<> float64 Property::getValue() const { return m_scalar.f64; }

template<> double2 Property::getValue() const { return *(double2*)m_data.data(); }
template<> double3 Property::getValue() const { return *(double3*)m_data.data(); }
template<> double4 Property::getValue() const { return *(double4*)m_data.data(); }
template<> double4x4 Property::getValue() const { return *(double4x4*)m_data.data(); }

template<> span<boolean> Property::getArray() const { return make_span((boolean*)m_data.data(), getArraySize()); }
template<> span<int16>   Property::getArray() const { return make_span((int16*)m_data.data(), getArraySize()); }
template<> span<int32>   Property::getArray() const { return make_span((int32*)m_data.data(), getArraySize()); }
template<> span<int64>   Property::getArray() const { return make_span((int64*)m_data.data(), getArraySize()); }
template<> span<float32> Property::getArray() const { return make_span((float32*)m_data.data(), getArraySize()); }
template<> span<float64> Property::getArray() const { return make_span((float64*)m_data.data(), getArraySize()); }

template<> span<float2>  Property::getArray() const { return make_span((float2*)m_data.data(), getArraySize() / 2); }
template<> span<float3>  Property::getArray() const { return make_span((float3*)m_data.data(), getArraySize() / 3); }
template<> span<float4>  Property::getArray() const { return make_span((float4*)m_data.data(), getArraySize() / 4); }
template<> span<double2> Property::getArray() const { return make_span((double2*)m_data.data(), getArraySize() / 2); }
template<> span<double3> Property::getArray() const { return make_span((double3*)m_data.data(), getArraySize() / 3); }
template<> span<double4> Property::getArray() const { return make_span((double4*)m_data.data(), getArraySize() / 4); }

std::string Property::getString() const
{
    return std::string(m_data.data(), m_data.size());
}

std::string Property::toString(int depth) const
{
    if (isArray()) {
        auto toS = [depth](const auto& span) {
            std::string s = "*";
            s += std::to_string(span.size());
            s += " {\n";
            AddTabs(s, depth + 1);
            s += "a: ";
            join(s, span, ",");
            s += "\n";
            AddTabs(s, depth);
            s += "}";
            return s;
        };
        switch (m_type) {
        case PropertyType::BoolArray: return toS(getArray<boolean>());
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
        case PropertyType::Bool: return std::to_string(getValue<boolean>());
        case PropertyType::Int16: return std::to_string(getValue<int16>());
        case PropertyType::Int32: return std::to_string(getValue<int32>());
        case PropertyType::Int64: return std::to_string(getValue<int64>());
        case PropertyType::Float32: return std::to_string(getValue<float32>());
        case PropertyType::Float64: return std::to_string(getValue<float64>());

        case PropertyType::Blob:
        {
            std::string s;
            s += '"';
            s += Base64Encode(make_span(m_data));
            s += '"';
            return s;
        }
        case PropertyType::String:
        {
            std::string s;
            s += " "; // just reserve space to avoid escape
            if (!m_data.empty()) {
                auto get_span = [](const char* s, size_t n) {
                    size_t i = 0;
                    for (; s[i] != '\0' && i < n; ++i) {}
                    return make_span(s, i);
                };

                span<char> first, second;
                {
                    size_t n = m_data.size();
                    const char* pos = m_data.data();
                    first = get_span(pos, n);

                    if (first.size() + 1 < n) {
                        n -= first.size() + 2;
                        pos += first.size() + 2;
                        second = get_span(pos, n);
                    }
                }
                if (!second.empty()) {
                    s.insert(s.end(), second.data(), second.data() + second.size());
                    s += "::";
                }
                s.insert(s.end(), first.data(), first.data() + first.size());
                Escape(s);
            }
            s[0] = '"'; // replace reserved space
            s += '"';
            return s;
        }

        default:
            break;
        }
    }
    return "";
}

} // namespace sfbx

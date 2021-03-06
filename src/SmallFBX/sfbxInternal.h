#pragma once
#include "sfbxTypes.h"
#include "sfbxAlgorithm.h"
#include "sfbxMath.h"
#include "sfbxTokens.h"
#include "sfbxNode.h"
#include "sfbxUtil.h"

namespace sfbx {

template<class T>
inline T read1(std::istream& is)
{
    static_assert(std::is_pod_v<T>);
    T r;
    is.read((char*)&r, sizeof(T));
    return r;
}

inline void readv(std::istream& is, void* dst, size_t size)
{
    is.read((char*)dst, size);
}

inline void readv(std::istream& is, std::string& dst, size_t s)
{
    dst.resize(s);
    is.read(dst.data(), s);
}

template<class T>
inline void write1(std::ostream& os, T v)
{
    static_assert(std::is_pod_v<T>);
    os.write((const char*)&v, sizeof(T));
}

inline void writev(std::ostream& os, const void* src, size_t size)
{
    os.write((const char*)src, size);
}


inline void AddTabs(std::string& dst, int n)
{
    for (int i = 0; i < n; ++i)
        dst += '\t';
}


inline PropertyType GetPropertyType(Node* node, size_t pi = 0)
{
    if (node)
        if (Property* prop = node->getProperty(pi))
            return prop->getType();
    return PropertyType::Unknown;
}

template<class T>
inline T GetPropertyValue(Node* node, size_t pi = 0)
{
    if (node)
        if (Property* prop = node->getProperty(pi))
            return prop->getValue<T>();
    return {};
}

template<class T>
inline span<T> GetPropertyArray(Node* node, size_t pi = 0)
{
    if (node)
        if (Property* prop = node->getProperty(pi))
            return prop->getArray<T>();
    return {};
}

inline string_view GetPropertyString(Node* node, size_t pi = 0)
{
    if (node)
        if (Property* prop = node->getProperty(pi))
            return prop->getString();
    return {};
}

template<class T>
inline T GetChildPropertyValue(Node* node, string_view name, size_t pi = 0)
{
    if (node)
        if (Node* child = node->findChild(name))
            if (Property* prop = child->getProperty(pi))
                return prop->getValue<T>();
    return {};
}

template<class T>
inline span<T> GetChildPropertyArray(Node* node, string_view name, size_t pi = 0)
{
    if (node)
        if (Node* child = node->findChild(name))
            if (Property* prop = child->getProperty(pi))
                return prop->getArray<T>();
    return {};
}

inline string_view GetChildPropertyString(Node* node, string_view name, size_t pi = 0)
{
    if (node)
        if (Node* child = node->findChild(name))
            if (Property* prop = child->getProperty(pi))
                return prop->getString();
    return {};
}

template<class Body>
inline void EnumerateProperties(Node* n, const Body& body)
{
    for (Node* props : n->getChildren()) {
        if (props->getName().starts_with(sfbxS_Properties)) {
            for (Node* p : props->getChildren())
                body(p);
            break;
        }
    }
}

class CounterStream : public std::ostream
{
public:
    CounterStream();
    uint64_t size();
    void reset();

private:
    class StreamBuf : public std::streambuf
    {
    public:
        static char s_dummy_buf[1024];

        StreamBuf();
        int overflow(int c) override;
        int sync() override;
        void reset();

        uint64_t m_size = 0;
    } m_buf;
};

} // namespace sfbx

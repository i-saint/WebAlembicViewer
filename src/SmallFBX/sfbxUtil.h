#pragma once

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


template<class Values, class Body>
void each(Values& val, const Body& body)
{
    for (auto& v : val)
        body(v);
}

template<class Values, class Indices, class Body>
void each_indexed(Values& val, Indices& idx, const Body& body)
{
    for (auto i : idx)
        body(val[i]);
}

template<class T>
void copy(T* dst, const T* src, size_t n)
{
    for (size_t i = 0; i < n; ++i)
        *dst++ = *src++;
}
template<class Dst, class Src>
void copy(Dst& dst, const Src& src)
{
    dst.resize(src.size());
    auto* d = dst.data();
    for (const auto& v : src)
        *d++ = v;
}
template<class T, class U>
void copy(span<T> dst, const span<U> src)
{
    auto* d = dst.data();
    for (const U& v : src)
        *d++ = v;
}

template<class Dst, class Src, class Indices>
void copy_indexed(Dst& dst, Src& src, Indices& idx)
{
    dst.resize(idx.size());
    auto* d = dst.data();
    for (auto i : idx)
        *d++ = src[i];
}

template<class String, class Container, class ToString>
void join(String& dst, const Container& cont, typename String::const_pointer sep, const ToString& to_s)
{
    bool first = true;
    for (auto& v : cont) {
        if (!first)
            dst += sep;
        dst += to_s(v);
        first = false;
    }
}
template<class Container>
void join(std::string& dst, const Container& cont, const char* sep)
{
    join(dst, cont, sep,
        [](typename Container::const_reference v) { return std::to_string(v); });
}

template<class Int, class Body>
void times(Int n, const Body& body)
{
    for (Int i = 0; i < n; ++i)
        body(i);
}


template<class T, class = void>
struct resize_impl
{
    void operator()(T&, size_t) {}
};
template<class T>
struct resize_impl<T, std::void_t<decltype(std::declval<T>().resize(0))>>
{
    void operator()(T& dst, size_t n) { dst.resize(n); }
};
template <class Cont>
inline void resize(Cont& cont, size_t n)
{
    resize_impl<Cont>()(cont, n);
}

template<class Dst, class Src, class Body>
inline void transform(Dst& dst, const Src& src, const Body& body)
{
    size_t c = src.size();
    resize(dst, c);
    for (size_t i = 0; i < c; ++i)
        dst[i] = body(src[i]);
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

inline std::string GetPropertyString(Node* node, size_t pi = 0)
{
    if (node)
        if (Property* prop = node->getProperty(pi))
            return prop->getString();
    return {};
}

template<class T>
inline T GetChildPropertyValue(Node* node, const char* name, size_t pi = 0)
{
    if (node)
        if (Node* child = node->findChild(name))
            if (Property* prop = child->getProperty(pi))
                return prop->getValue<T>();
    return {};
}

template<class T>
inline span<T> GetChildPropertyArray(Node* node, const char* name, size_t pi = 0)
{
    if (node)
        if (Node* child = node->findChild(name))
            if (Property* prop = child->getProperty(pi))
                return prop->getArray<T>();
    return {};
}

inline std::string GetChildPropertyString(Node* node, const char* name, size_t pi = 0)
{
    if (node)
        if (Node* child = node->findChild(name))
            if (Property* prop = child->getProperty(pi))
                return prop->getString();
    return {};
}

inline void AddTabs(std::string& dst, int n)
{
    for (int i = 0; i < n; ++i)
        dst += '\t';
}

} // namespace sfbx

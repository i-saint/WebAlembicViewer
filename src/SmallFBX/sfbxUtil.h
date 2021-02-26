#pragma once
#include <iostream>
#include "sfbxTypes.h"
#include "sfbxRawVector.h"

namespace sfbx {

template<class T>
inline T read1(std::istream& is)
{
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
    os.write((const char*)&v, sizeof(T));
}

inline void write1(std::ostream& os, const char* s)
{
    os.write(s, std::strlen(s));
}

inline void writev(std::ostream& os, const void* src, size_t size)
{
    os.write((const char*)src, size);
}

} // namespace sfbx

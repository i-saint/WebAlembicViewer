#pragma once
#include <iostream>

namespace fbx {

template<class T>
inline T read1(std::istream& is)
{
    T r;
    is.read((char*)&r, sizeof(T));
    return r;
}
inline void read_s(std::istream& is, std::string& dst, size_t s)
{
    dst.resize(s);
    is.read(dst.data(), s);
}

template<class T>
inline void write1(std::ostream& os, T v)
{
    os.write((char*)&v, sizeof(T));
}

inline void write_s(std::ostream& os, const char* s)
{
    os.write(s, std::strlen(s));
}

} // namespace fbx

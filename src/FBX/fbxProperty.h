#pragma once

#include <memory>
#include <iostream>
#include <vector>

namespace fbx {

class FBXProperty
{
public:
    explicit FBXProperty(std::istream &input);
    explicit FBXProperty(const std::vector<char>& v, uint8_t type);
    explicit FBXProperty(const char*);
    template<class T> explicit FBXProperty(const T& v);

    void read(std::istream& input);
    void write(std::ostream& output);

    template<class T> void operator=(const T& v);
    void operator=(const char* v);

    std::string toString() const;
    char getType() const;
    bool isArray() const;
    uint32_t getArraySize() const;
    uint32_t getBytes() const;

    template<class T> T getScalar() const;
    template<class T> std::span<T> getArray() const;
    std::string getString() const;

private:
    uint8_t m_type;
    std::vector<char> m_data;
};

} // namespace fbx

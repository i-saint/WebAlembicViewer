#pragma once
#include "sfbxProperty.h"

namespace sfbx {

class Node
{
public:
    Node(std::string name = "");

    std::uint32_t read(std::istream &input, uint32_t start_offset);
    std::uint32_t write(std::ostream &output, uint32_t start_offset);
    bool isNull();

    void addProperty(Property&& v);
    void addProperty(PropertyType t, const std::vector<char>& v) { addProperty(Property(t, v)); }
    void addProperty(const char* v) { addProperty(Property(v)); }
    template<class T> void addProperty(const T& v) { addProperty(Property(v)); }
    template<class T> void addProperty(const std::vector<T>& v) { addProperty(Property(v)); }

    void addChild(Node&& child);

    const std::string& getName() const;
    uint32_t getSizeInBytes() const;
    const std::vector<Property>& getProperties();
    const std::vector<Node>& getChildren();

private:
    std::string m_name;
    std::vector<Property> m_properties;
    std::vector<Node> m_children;
};

} // namespace sfbx

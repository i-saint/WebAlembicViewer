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

    void addProperty(PropertyPtr v);
    void addProperty(PropertyType t, const std::vector<char>& v) { addProperty(MakeProperty(t, v)); }
    void addProperty(const char* v) { addProperty(MakeProperty(v)); }
    template<class T> void addProperty(const T& v) { addProperty(MakeProperty(v)); }
    template<class T> void addProperty(const std::vector<T>& v) { addProperty(MakeProperty(v)); }

    void addChild(NodePtr child);

    const std::string& getName() const;
    uint32_t getSizeInBytes() const;
    const std::vector<PropertyPtr>& getProperties();
    const std::vector<NodePtr>& getChildren();

private:
    std::string m_name;
    std::vector<PropertyPtr> m_properties;
    std::vector<NodePtr> m_children;
};

template<class... T>
inline NodePtr MakeNode(T&&... v)
{
    return std::make_shared<Node>(std::forward<T>(v)...);
}

} // namespace sfbx

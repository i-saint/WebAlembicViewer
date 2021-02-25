#pragma once
#include "sfbxProperty.h"

namespace sfbx {

class Node
{
public:
    Node(const std::string& name = "");

    std::uint32_t read(std::istream &input, uint32_t start_offset);
    std::uint32_t write(std::ostream &output, uint32_t start_offset);
    bool isNull();

    void addProperty(PropertyPtr v);
    template<class... T>
    void addProperty(T&&... v) { addProperty(MakeProperty(std::forward<T>(v)...)); }

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

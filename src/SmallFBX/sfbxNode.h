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

    void setName(const std::string& v);
    void addProperty(PropertyPtr v);
    template<class... T>
    void addProperty(T&&... v) { addProperty(MakeProperty(std::forward<T>(v)...)); }
    void addChild(NodePtr child);

    uint32_t getSizeInBytes() const;
    const std::string& getName() const;
    const std::vector<PropertyPtr>& getProperties() const;
    const std::vector<NodePtr>& getChildren() const;

    NodePtr findChild(const char* name) const;
    NodePtr findChild(const std::string& name) const { return findChild(name.c_str()); }
    PropertyPtr getProperty(size_t i);
    PropertyPtr findChildProperty(const char* name, size_t i) const { return findChild(name)->getProperty(i); }
    PropertyPtr findChildProperty(const std::string& name, size_t i) const { return findChildProperty(name.c_str(), i); }

    template<class Body>
    void eachChild(const Body& body)
    {
        for (auto& c : m_children)
            if (!c->isNull())
                body(c);
    }

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

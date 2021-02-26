#pragma once
#include "sfbxProperty.h"

namespace sfbx {

class Node
{
friend class Document;
public:
    Node(Document* doc = nullptr, const std::string& name = "");

    std::uint32_t read(std::istream &input, uint32_t start_offset);
    std::uint32_t write(std::ostream &output, uint32_t start_offset);
    bool isNull();

    void setName(const std::string& v);

    Property* createProperty();
    template<class... T>
    Property* addProperty(T&&... v) { auto p = createProperty(); p->assign(std::forward<T>(v)...); return p; }

    Node* createNode(const char* name = "");
    Node* createNode(const std::string& name) { return createNode(name.c_str()); }

    uint32_t getSizeInBytes() const;
    const std::string& getName() const;
    span<Property*> getProperties() const;

    Node* getParent() const;
    span<Node*> getChildren() const;

    Node* findChild(const char* name) const;
    Node* findChild(const std::string& name) const { return findChild(name.c_str()); }
    Property* getProperty(size_t i);
    Property* findChildProperty(const char* name, size_t i = 0) const { return findChild(name)->getProperty(i); }
    Property* findChildProperty(const std::string& name, size_t i = 0) const { return findChildProperty(name.c_str(), i); }

    template<class Body>
    void eachChild(const Body& body)
    {
        for (auto c : m_children)
            if (!c->isNull())
                body(c);
    }

private:
    Document* m_document{};
    std::string m_name;
    std::vector<Property*> m_properties;

    Node* m_parent{};
    std::vector<Node*> m_children;
};

template<class... T>
inline NodePtr MakeNode(T&&... v)
{
    return std::make_shared<Node>(std::forward<T>(v)...);
}

} // namespace sfbx

#pragma once
#include "sfbxProperty.h"

namespace sfbx {

class Node
{
friend class Document;
private:
    Node();

public:
    std::uint32_t read(std::istream &input, uint32_t start_offset);
    std::uint32_t write(std::ostream &output, uint32_t start_offset);
    bool isNull();

    void setName(const std::string& v);

    Property* createProperty();
    template<class... T>
    void addProperty(T&&... v) { createProperty()->assign(std::forward<T>(v)...); }

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

private:
    Document* m_document{};
    std::string m_name;
    std::vector<Property*> m_properties;

    Node* m_parent{};
    std::vector<Node*> m_children;
};

} // namespace sfbx

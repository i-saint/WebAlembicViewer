#pragma once
#include "sfbxProperty.h"

namespace sfbx {

class Node
{
friend class Document;
private:
    Node();

public:
    uint64_t read(std::istream &input, uint64_t start_offset);
    uint64_t write(std::ostream &output, uint64_t start_offset);
    bool isNull();

    void setName(const std::string& v);

    Property* createProperty();
    template<class... T> void addProperty(T&&... v) { createProperty()->assign(std::forward<T>(v)...); }

    Node* createNode(const char* name = "");
    Node* createNode(const std::string& name) { return createNode(name.c_str()); }
    template<class... T> void addPropertyNode(const char* name, T&&... v) { createNode(name)->addProperty(std::forward<T>(v)...);  }
    template<class... T> void addPropertyNode(const std::string& name, T&&... v) { createNode(name.c_str())->addProperty(std::forward<T>(v)...); }

    uint64_t getSizeInBytes() const;
    const std::string& getName() const;

    span<Property*> getProperties() const;
    Property* getProperty(size_t i);

    Node* getParent() const;
    span<Node*> getChildren() const;
    Node* getChild(size_t i) const;
    Node* findChild(const char* name) const;
    Node* findChild(const std::string& name) const { return findChild(name.c_str()); }
    
    Property* findChildProperty(const char* name, size_t i = 0) const { return findChild(name)->getProperty(i); }
    Property* findChildProperty(const std::string& name, size_t i = 0) const { return findChildProperty(name.c_str(), i); }

private:
    uint32_t getDocumentVersion() const;
    uint32_t getHeaderSize() const;

    Document* m_document{};
    std::string m_name;
    std::vector<Property*> m_properties;

    Node* m_parent{};
    std::vector<Node*> m_children;
};

} // namespace sfbx

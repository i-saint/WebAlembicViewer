#pragma once
#include "sfbxProperty.h"

namespace sfbx {

class Node
{
friend class Document;
public:
    uint64_t read(std::istream &input, uint64_t start_offset);
    uint64_t write(std::ostream &output, uint64_t start_offset);
    bool isNull() const;
    bool isRoot() const;

    void setName(const std::string& v);

    Property* createProperty();
    Node* createChild(const std::string& name = "");
    void eraseChild(Node* n);

    // utils
    template<class T> Property* addProperty(const T& v) { auto r = createProperty(); r->assign(v); return r; }
    void addProperties() {}
    template<class T, class... U> void addProperties(T&& v, U&&... a) { addProperty(v); addProperties(a...); }
    template<class... T> Node* createChild(const std::string& name, T&&... v) { auto r = createChild(name);  r->addProperties(v...); return r; }


    const std::string& getName() const;

    span<PropertyPtr> getProperties() const;
    Property* getProperty(size_t i);

    Node* getParent() const;
    span<Node*> getChildren() const;
    Node* getChild(size_t i) const;
    Node* findChild(const char* name) const;
    Node* findChild(const std::string& name) const { return findChild(name.c_str()); }

    std::string toString(int depth = 0) const;

private:
    Node(const Node&) = delete;
    Node& operator=(const Node) = delete;
    Node();
    uint32_t getDocumentVersion() const;
    uint32_t getHeaderSize() const;

    Document* m_document{};
    std::string m_name;
    std::vector<PropertyPtr> m_properties;

    Node* m_parent{};
    std::vector<Node*> m_children;
};

} // namespace sfbx

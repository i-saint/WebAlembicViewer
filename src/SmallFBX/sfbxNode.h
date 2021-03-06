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

    void setName(string_view v);

    Property* createProperty();
    Node* createChild(string_view name = {});
    void eraseChild(Node* n);

    // utils
    template<class T> Property* addProperty(const T& v) { auto r = createProperty(); r->assign(v); return r; }
    void addProperties() {}
    template<class T, class... U> void addProperties(T&& v, U&&... a) { addProperty(v); addProperties(a...); }
    template<class... T> Node* createChild(string_view name, T&&... v) { auto r = createChild(name);  r->addProperties(v...); return r; }


    string_view getName() const;

    span<PropertyPtr> getProperties() const;
    Property* getProperty(size_t i);
    // for legacy format. there are no array types and arrays are represented as a huge list of properties.
    template<class Dst, class Src> void getPropertiesValues(RawVector<Dst>& dst) const;

    Node* getParent() const;
    span<Node*> getChildren() const;
    Node* getChild(size_t i) const;
    Node* findChild(string_view name) const;

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

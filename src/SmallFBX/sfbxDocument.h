#pragma once

#include "sfbxNode.h"
#include "sfbxObject.h"

namespace sfbx {

class Document
{
public:
    Document();
    void read(std::istream &input);
    void write(std::ostream& output);

    void read(const std::string& fname);
    void write(const std::string& fname);

    uint32_t getVersion();

    Property* createProperty();
    template<class T>
    Property* createProperty(const T& v) { auto p = createProperty(); *p = v; return p; }

    Node* createNode(const char* name = "");
    Node* createNode(const std::string& name) { return createNode(name.c_str()); }
    Node* createChildNode(const char* name = "");
    Node* createChildNode(const std::string& name) { return createChildNode(name.c_str()); }
    Node* findNode(const char* name) const;
    Node* findNode(const std::string& name) const { return findNode(name.c_str()); }
    span<Node*> getRootNodes();

    Object* createObject(ObjectType t);
    Object* findObject(int64 id);
    span<Object*> getRootObjects();

    void createBasicStructure();


private:
    std::vector<PropertyPtr> m_properties;

    std::vector<NodePtr> m_nodes;
    std::vector<Node*> m_root_nodes;

    std::vector<ObjectPtr> m_objects;
    std::vector<Object*> m_root_objects;

    uint32_t m_version;
};

template<class... T>
inline DocumentPtr MakeDocument(T&&... v)
{
    return std::make_shared<Document>(std::forward<T>(v)...);
}

} // namespace sfbx

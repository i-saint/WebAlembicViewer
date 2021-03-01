#pragma once

#include "sfbxNode.h"
#include "sfbxObject.h"

namespace sfbx {

enum class FileVersion : int
{
    Unknown = 0,

    Fbx2014 = 7400,
    Fbx2015 = Fbx2014,

    Fbx2016 = 7500,
    Fbx2017 = Fbx2016,
    Fbx2018 = Fbx2016,

    Fbx2019 = 7600,
    Fbx2020 = Fbx2019,

    Default = Fbx2020,
};

class Document
{
public:
    Document();
    void read(std::istream &input);
    void read(const std::string& path);
    void write(std::ostream& output);
    void write(const std::string& path);

    FileVersion getVersion();
    void setVersion(FileVersion v);

    Property* createProperty();

    Node* createNode(const std::string& name = "");
    Node* createChildNode(const std::string& name = "");
    void registerNode(NodePtr n);
    Node* findNode(const char* name) const;
    Node* findNode(const std::string& name) const { return findNode(name.c_str()); }
    span<Node*> getRootNodes();

    Object* createObject(ObjectType t, ObjectSubType s);
    template<class T> T* createObject();
    Object* findObject(int64 id);
    span<Object*> getRootObjects();

    void constructNodes();

private:
    void createHeaderExtention();

    FileVersion m_version = FileVersion::Default;

    std::vector<PropertyPtr> m_properties;

    std::vector<NodePtr> m_nodes;
    std::vector<Node*> m_root_nodes;

    std::vector<ObjectPtr> m_objects;
    std::vector<Object*> m_root_objects;
};

template<class... T>
inline DocumentPtr MakeDocument(T&&... v)
{
    return std::make_shared<Document>(std::forward<T>(v)...);
}

} // namespace sfbx

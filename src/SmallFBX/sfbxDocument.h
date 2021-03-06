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

    Fbx2019 = 7700,
    Fbx2020 = Fbx2019,

    Default = Fbx2020,
};

class Document
{
public:
    Document();
    bool read(std::istream &input);
    bool read(const std::string& path);
    bool writeBinary(std::ostream& output);
    bool writeBinary(const std::string& path);
    bool writeAscii(std::ostream& output);
    bool writeAscii(const std::string& path);
    void unload();

    FileVersion getVersion();
    void setVersion(FileVersion v);

    Node* createNode(const std::string& name = "");
    Node* createChildNode(const std::string& name = "");
    void eraseNode(Node* n);
    Node* findNode(const char* name) const;
    Node* findNode(const std::string& name) const { return findNode(name.c_str()); }
    span<NodePtr> getAllNodes() const;
    span<Node*> getRootNodes() const;

    Object* createObject(ObjectClass t, ObjectSubClass s);
    template<class T> T* createObject(const std::string& name = "");
    Object* findObject(int64 id) const;
    span<ObjectPtr> getAllObjects() const;
    span<Object*> getRootObjects() const;
    Model* getRootModel() const;

    void constructNodes();
    std::string toString();

private:
    void initialize();

    FileVersion m_version = FileVersion::Default;

    std::vector<NodePtr> m_nodes;
    std::vector<Node*> m_root_nodes;

    std::vector<ObjectPtr> m_objects;
    std::vector<Object*> m_root_objects;
    Model* m_root_model{};
};

template<class... T>
inline DocumentPtr MakeDocument(T&&... v)
{
    return std::make_shared<Document>(std::forward<T>(v)...);
}

} // namespace sfbx

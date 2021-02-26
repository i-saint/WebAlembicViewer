#pragma once

#include "sfbxNode.h"
#include "sfbxGeometry.h"

namespace sfbx {

class Document
{
public:
    Document();
    void read(std::istream &input);
    void write(std::ostream& output);

    void read(const std::string& fname);
    void write(const std::string& fname);

    NodePtr findNode(const char* name) const;
    NodePtr findNode(const std::string& name) const { return findNode(name.c_str()); }

    void createBasicStructure();

    uint32_t getVersion();

public:
    std::vector<NodePtr> m_nodes;
    std::vector<GeometryPtr> m_geometries;
    uint32_t m_version;
};

template<class... T>
inline DocumentPtr MakeDocument(T&&... v)
{
    return std::make_shared<Document>(std::forward<T>(v)...);
}

} // namespace sfbx

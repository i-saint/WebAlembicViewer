#pragma once
#include "fbxNode.h"

namespace fbx {

class FBXDocument
{
public:
    FBXDocument();
    void read(std::istream &input);
    void write(std::ostream& output);

    void read(const std::string& fname);
    void write(const std::string& fname);

    void createBasicStructure();

    std::uint32_t getVersion();

public:
    std::vector<FBXNode> m_nodes;
    std::uint32_t m_version;
};

} // namespace fbx

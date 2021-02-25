#pragma once
#include "sfbxNode.h"

namespace sfbx {

class Document
{
public:
    Document();
    void read(std::istream &input);
    void write(std::ostream& output);

    void read(const std::string& fname);
    void write(const std::string& fname);

    void createBasicStructure();

    std::uint32_t getVersion();

public:
    std::vector<Node> m_nodes;
    std::uint32_t m_version;
};

} // namespace sfbx

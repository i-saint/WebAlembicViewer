#include "pch.h"
#include "fbxNode.h"
#include "fbxUtil.h"

namespace fbx {

FBXNode::FBXNode(std::string name)
    : m_name(name)
{
}

uint32_t FBXNode::read(std::istream& is, uint32_t start_offset)
{
    uint32_t bytes = 0;

    uint32_t endOffset = read1<uint32_t>(is);
    uint32_t numProperties = read1<uint32_t>(is);
    uint32_t propertyListLength = read1<uint32_t>(is);
    uint8_t nameLength = read1<uint8_t>(is);
    read_s(is, m_name, nameLength);
    bytes += 13 + nameLength;

    for (uint32_t i = 0; i < numProperties; i++) {
        addProperty(FBXProperty(is));
    }
    bytes += propertyListLength;

    while (start_offset + bytes < endOffset) {
        FBXNode child;
        bytes += child.read(is, start_offset + bytes);
        addChild(std::move(child));
    }
    return bytes;
}

uint32_t FBXNode::write(std::ostream& os, uint32_t start_offset)
{
    if (isNull()) {
        for (int i = 0; i < 13; i++)
            write1(os, (uint8_t)0);
        return 13;
    }

    uint32_t propertyListLength = 0;
    for (auto prop : m_properties) propertyListLength += prop.getBytes();
    uint32_t bytes = 13 + m_name.length() + propertyListLength;
    for (auto child : m_children) bytes += child.getBytes();

    if (bytes != getBytes())
        throw std::runtime_error("bytes != getBytes()");

    write1(os, start_offset + bytes); // endOffset
    write1(os, (uint32_t)m_properties.size()); // numProperties
    write1(os, propertyListLength); // propertyListLength
    write1(os, (uint8_t)m_name.length());
    write1(os, m_name);

    bytes = 13 + m_name.length() + propertyListLength;

    for (auto prop : m_properties) prop.write(os);
    for (auto child : m_children) bytes += child.write(os, start_offset + bytes);

    return bytes;
}

bool FBXNode::isNull()
{
    return m_children.size() == 0
            && m_properties.size() == 0
            && m_name.length() == 0;
}

void FBXNode::addProperty(const FBXProperty& v)
{
    m_properties.push_back(v);
}

void FBXNode::addChild(FBXNode child)
{
    m_children.push_back(child);
}

uint32_t FBXNode::getBytes() const
{
    uint32_t bytes = 13 + m_name.length();
    for(auto& child : m_children) {
        bytes += child.getBytes();
    }
    for(auto& prop : m_properties) {
        bytes += prop.getBytes();
    }
    return bytes;
}

const std::string& FBXNode::getName() const
{
    return m_name;
}

const std::vector<FBXProperty>& FBXNode::getProperties()
{
    return m_properties;
}

const std::vector<FBXNode>& FBXNode::getChildren()
{
    return m_children;
}

} // namespace fbx

#include "pch.h"
#include "sfbxNode.h"
#include "sfbxUtil.h"

namespace sfbx {

Node::Node(std::string name)
    : m_name(name)
{
}

uint32_t Node::read(std::istream& is, uint32_t start_offset)
{
    uint32_t ret = 0;

    uint32_t end_offset = read1<uint32_t>(is);
    uint32_t num_props = read1<uint32_t>(is);
    uint32_t prop_size = read1<uint32_t>(is);
    uint8_t name_len = read1<uint8_t>(is);
    readv(is, m_name, name_len);
    ret += 13 + name_len;

    for (uint32_t i = 0; i < num_props; i++)
        addProperty(Property(is));
    ret += prop_size;

    while (start_offset + ret < end_offset) {
        Node child;
        ret += child.read(is, start_offset + ret);
        addChild(std::move(child));
    }
    return ret;
}

uint32_t Node::write(std::ostream& os, uint32_t start_offset)
{
    if (isNull()) {
        for (int i = 0; i < 13; i++)
            write1(os, (uint8_t)0);
        return 13;
    }

    uint32_t property_size = 0;
    for (auto& prop : m_properties)
        property_size += prop.getBytes();

    uint32_t ret = 13 + m_name.length() + property_size;
    for (auto& child : m_children)
        ret += child.getBytes();

    if (ret != getBytes())
        throw std::runtime_error("bytes != getBytes()");

    write1(os, uint32_t(start_offset + ret));
    write1(os, uint32_t(m_properties.size()));
    write1(os, uint32_t(property_size));
    write1(os, uint8_t(m_name.length()));
    write1(os, m_name);

    ret = 13 + m_name.length() + property_size;

    for (auto& prop : m_properties)
        prop.write(os);
    for (auto& child : m_children)
        ret += child.write(os, start_offset + ret);

    return ret;
}

bool Node::isNull()
{
    return m_children.size() == 0
            && m_properties.size() == 0
            && m_name.length() == 0;
}

void Node::addProperty(Property&& v)
{
    m_properties.push_back(v);
}

void Node::addChild(Node&& child)
{
    m_children.push_back(child);
}

uint32_t Node::getBytes() const
{
    uint32_t ret = 13 + m_name.length();
    for (auto& child : m_children)
        ret += child.getBytes();
    for (auto& prop : m_properties)
        ret += prop.getBytes();
    return ret;
}

const std::string& Node::getName() const
{
    return m_name;
}

const std::vector<Property>& Node::getProperties()
{
    return m_properties;
}

const std::vector<Node>& Node::getChildren()
{
    return m_children;
}

} // namespace sfbx

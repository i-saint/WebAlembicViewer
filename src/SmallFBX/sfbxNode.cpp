#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxNode.h"
#include "sfbxDocument.h"

namespace sfbx {

Node::Node()
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

    for (uint32_t i = 0; i < num_props; i++) {
        auto p = createProperty();
        p->read(is);
    }
    ret += prop_size;

    while (start_offset + ret < end_offset) {
        auto child = createNode();
        ret += child->read(is, start_offset + ret);
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
        property_size += prop->getSizeInBytes();

    uint32_t ret = 13 + m_name.length() + property_size;
    for (auto& child : m_children)
        ret += child->getSizeInBytes();

    if (ret != getSizeInBytes())
        throw std::runtime_error("sfbx::Node::write1(): ret != getSizeInBytes()");

    write1(os, uint32_t(start_offset + ret));
    write1(os, uint32_t(m_properties.size()));
    write1(os, uint32_t(property_size));
    write1(os, uint8_t(m_name.length()));
    write1(os, m_name);

    ret = 13 + m_name.length() + property_size;

    for (auto& prop : m_properties)
        prop->write(os);
    for (auto& child : m_children)
        ret += child->write(os, start_offset + ret);

    return ret;
}

bool Node::isNull()
{
    return m_name.empty() && m_children.empty() && m_properties.empty();
}

void Node::setName(const std::string& v)
{
    m_name = v;
}

Property* Node::createProperty()
{
    auto p = m_document->createProperty();
    m_properties.push_back(p);
    return p;
}

Node* Node::createNode(const char* name)
{
    auto p = m_document->createChildNode(name);
    m_children.push_back(p);
    p->m_parent = this;
    return p;
}

uint32_t Node::getSizeInBytes() const
{
    uint32_t ret = 13 + m_name.length();
    for (auto& child : m_children)
        ret += child->getSizeInBytes();
    for (auto& prop : m_properties)
        ret += prop->getSizeInBytes();
    return ret;
}

const std::string& Node::getName() const
{
    if (!this) {
        static std::string s_dummy;
        return s_dummy;
    }
    return m_name;
}

span<Property*> Node::getProperties() const
{
    if (!this)
        return {};
    return make_span(m_properties);
}

Property* Node::getProperty(size_t i)
{
    if (!this)
        return {};
    if (this && i < m_properties.size())
        return m_properties[i];
    return nullptr;
}

Node* Node::getParent() const
{
    if (!this)
        return {};
    return m_parent;
}

span<Node*> Node::getChildren() const
{
    if (!this)
        return {};
    return make_span(m_children);
}

Node* Node::getChild(size_t i) const
{
    if (!this)
        return {};
    return i < m_children.size() ? m_children[i] : nullptr;
}

Node* Node::findChild(const char* name) const
{
    if (!this)
        return {};
    auto it = std::find_if(m_children.begin(), m_children.end(),
        [name](Node* p) { return p->getName() == name; });
    return it != m_children.end() ? *it : nullptr;
}

} // namespace sfbx

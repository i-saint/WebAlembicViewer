#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxNode.h"
#include "sfbxDocument.h"

namespace sfbx {

Node::Node()
{
}

uint64_t Node::read(std::istream& is, uint64_t start_offset)
{
    uint64_t ret = 0;

    uint64_t end_offset, num_props, prop_size;
    if (getDocumentVersion() >= sfbxI_FBX2016_FileVersion) {
        // size records are 64bit since FBX 2016
        end_offset = read1<uint64_t>(is);
        num_props = read1<uint64_t>(is);
        prop_size = read1<uint64_t>(is);
        ret += 24;
    }
    else {
        end_offset = read1<uint32_t>(is);
        num_props = read1<uint32_t>(is);
        prop_size = read1<uint32_t>(is);
        ret += 12;
    }

    uint8_t name_len = read1<uint8_t>(is);
    readv(is, m_name, name_len);
    ret += 1;
    ret += name_len;

    for (uint32_t i = 0; i < num_props; i++) {
        auto p = createProperty();
        p->read(is);
    }
    ret += prop_size;

    while (start_offset + ret < end_offset) {
        auto child = NodePtr(new Node());
        child->m_document = m_document;
        child->m_parent = this;
        ret += child->read(is, start_offset + ret);
        if (!child->isNull()) {
            m_document->registerNode(child);
            m_children.push_back(child.get());
        }
        else {
            printf("");
        }
    }
    return ret;
}

uint64_t Node::write(std::ostream& os, uint64_t start_offset)
{
    uint32_t header_size = getHeaderSize();
    if (isNull()) {
        for (uint32_t i = 0; i < header_size; i++)
            write1(os, (uint8_t)0);
        return header_size;
    }

    uint64_t property_size = 0;
    for (auto prop : m_properties)
        property_size += prop->getSizeInBytes();

    uint64_t pos = header_size + m_name.length() + property_size;
    for (auto child : m_children)
        pos += child->getSizeInBytes();

    if (getDocumentVersion() >= sfbxI_FBX2016_FileVersion) {
        // size records are 64bit since FBX 2016
        write1(os, uint64_t(start_offset + pos));
        write1(os, uint64_t(m_properties.size()));
        write1(os, uint64_t(property_size));
    }
    else {
        write1(os, uint32_t(start_offset + pos));
        write1(os, uint32_t(m_properties.size()));
        write1(os, uint32_t(property_size));
    }
    write1(os, uint8_t(m_name.size()));
    writev(os, m_name.data(), m_name.size());

    pos = header_size + m_name.length() + property_size;
    for (auto prop : m_properties)
        prop->write(os);
    for (auto child : m_children)
        pos += child->write(os, start_offset + pos);

    return pos;
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

Node* Node::createChild(const std::string& name)
{
    auto p = m_document->createChildNode(name);
    m_children.push_back(p);
    p->m_parent = this;
    return p;
}

uint64_t Node::getSizeInBytes() const
{
    uint64_t ret = getHeaderSize() + m_name.length();
    for (auto& prop : m_properties)
        ret += prop->getSizeInBytes();
    for (auto& child : m_children)
        ret += child->getSizeInBytes();
    return ret;
}

const std::string& Node::getName() const
{
    return m_name;
}

span<Property*> Node::getProperties() const
{
    return make_span(m_properties);
}

Property* Node::getProperty(size_t i)
{
    if (i < m_properties.size())
        return m_properties[i];
    return nullptr;
}

Node* Node::getParent() const
{
    return m_parent;
}

span<Node*> Node::getChildren() const
{
    return make_span(m_children);
}

Node* Node::getChild(size_t i) const
{
    return i < m_children.size() ? m_children[i] : nullptr;
}

Node* Node::findChild(const char* name) const
{
    auto it = std::find_if(m_children.begin(), m_children.end(),
        [name](Node* p) { return p->getName() == name; });
    return it != m_children.end() ? *it : nullptr;
}

uint32_t Node::getDocumentVersion() const
{
    return (uint32_t)m_document->getVersion();
}

uint32_t Node::getHeaderSize() const
{
    if (getDocumentVersion() >= sfbxI_FBX2016_FileVersion) {
        // sizeof(uint64_t) * 3 + 1
        return 25;
    }
    else {
        // sizeof(uint32_t) * 3 + 1
        return 13;
    }
}

} // namespace sfbx

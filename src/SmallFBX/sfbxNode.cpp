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
        auto child = createChild();
        ret += child->read(is, start_offset + ret);
        if (child->isNull())
            eraseChild(child);
    }
    return ret;
}

uint64_t Node::write(std::ostream& os, uint64_t start_offset)
{
    Node null_node;
    null_node.m_document = m_document;

    uint32_t header_size = getHeaderSize() + m_name.size();
    uint64_t property_size = 0;
    uint64_t children_size = 0;
    if (isNull()) {
        for (uint32_t i = 0; i < header_size; i++)
            write1(os, (uint8_t)0);
        return header_size;
    }

    {
        CounterStream cs;
        for (auto prop : m_properties)
            prop->write(cs);
        property_size = cs.size();
    }
    {
        CounterStream cs;
        for (auto child : m_children)
            child->write(cs, 0);
        null_node.write(cs, 0);
        children_size = cs.size();
    }

    uint64_t end_offset = start_offset + header_size + property_size + children_size;
    if (getDocumentVersion() >= sfbxI_FBX2016_FileVersion) {
        // size records are 64bit since FBX 2016
        write1(os, uint64_t(end_offset));
        write1(os, uint64_t(m_properties.size()));
        write1(os, uint64_t(property_size));
    }
    else {
        write1(os, uint32_t(end_offset));
        write1(os, uint32_t(m_properties.size()));
        write1(os, uint32_t(property_size));
    }
    write1(os, uint8_t(m_name.size()));
    writev(os, m_name.data(), m_name.size());

    for (auto prop : m_properties)
        prop->write(os);

    uint64_t pos = header_size + property_size;
    for (auto child : m_children)
        pos += child->write(os, start_offset + pos);
    pos += null_node.write(os, start_offset + pos);
    return pos;
}

bool Node::isNull() const
{
    return m_name.empty() && m_children.empty() && m_properties.empty();
}

bool Node::isRoot() const
{
    return m_parent == nullptr;
}

void Node::setName(const std::string& v)
{
    m_name = v;
}

Property* Node::createProperty()
{
    auto p = std::make_shared<Property>();
    m_properties.push_back(p);
    return p.get();
}

Node* Node::createChild(const std::string& name)
{
    auto p = m_document->createChildNode(name);
    m_children.push_back(p);
    p->m_parent = this;
    return p;
}

void Node::eraseChild(Node* n)
{
    m_document->eraseNode(n);

    auto it = std::find(m_children.begin(), m_children.end(), n);
    if (it != m_children.end())
        m_children.erase(it);
}

const std::string& Node::getName() const
{
    return m_name;
}

span<PropertyPtr> Node::getProperties() const
{
    return make_span(m_properties);
}

Property* Node::getProperty(size_t i)
{
    if (i < m_properties.size())
        return m_properties[i].get();
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

std::string Node::toString(int depth) const
{
    std::string s;
    AddTabs(s, depth);
    s += getName();
    s += ": ";
    join(s, m_properties, ", ",
        [depth](auto& p) { return p->toString(depth); });
    s += " ";

    if (!m_children.empty() || (m_children.empty() && m_properties.empty())) {
        s += "{\n";
        for (auto* c : m_children)
            s += c->toString(depth + 1);
        AddTabs(s, depth);
        s += "}";
    }
    s += "\n";

    return s;
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

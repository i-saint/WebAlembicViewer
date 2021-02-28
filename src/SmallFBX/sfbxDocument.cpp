#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxDocument.h"

namespace sfbx {

Document::Document()
{
}

static const char g_fbx_magic[23]{
    'K', 'a', 'y', 'd', 'a', 'r', 'a', ' ', 'F', 'B', 'X', ' ', 'B', 'i', 'n', 'a', 'r', 'y', ' ', ' ', '\x00', '\x1A', '\x00',
};

void Document::read(std::istream& is)
{
    char magic[23];
    readv(is, magic, 23);
    if (memcmp(magic, g_fbx_magic, 23) != 0) {
        throw std::runtime_error("Not a FBX file");
    }

    m_version = (FileVersion)read1<uint32_t>(is);

    uint64_t pos = 27; // magic (23) + version (4)
    for (;;) {
        auto node = createNode();
        pos += node->read(is, pos);
        if (node->isNull())
            break;
    }

    if (auto objects = findNode(sfbxS_Objects)) {
        for (auto n : objects->getChildren()) {
            if (auto obj = createObject(GetFbxObjectType(n->getName()))) {
                obj->setNode(n);
            }
        }
    }

    if (auto connections = findNode(sfbxS_Connections)) {
        for (auto n : connections->getChildren()) {
            if (n->getName() == sfbxS_C && GetPropertyString(n, 0) == sfbxS_OO) {
                int64 cid = GetPropertyValue<int64>(n, 1);
                int64 pid = GetPropertyValue<int64>(n, 2);
                Object* child = findObject(cid);
                Object* parent = findObject(pid);
                if (child && parent)
                    parent->addChild(child);
            }
        }
    }

    for (auto& obj : m_objects) {
        obj->constructObject();
        if (obj->getParents().empty())
            m_root_objects.push_back(obj.get());
    }
}

void Document::read(const std::string& path)
{
    std::ifstream file;
    file.open(path, std::ios::in | std::ios::binary);
    if (file) {
        read(file);
    }
    else {
        throw std::runtime_error("Cannot read from file: \"" + path + "\"");
    }
}


void Document::write(std::ostream& os)
{
    constructNodes();

    writev(os, g_fbx_magic, 23);
    write1(os, m_version);

    uint32_t offset = 27; // magic: 21+2, version: 4
    for (auto node : m_root_nodes)
        offset += node->write(os, offset);

    Node null_node;
    null_node.m_document = this;
    offset += null_node.write(os, offset);

    uint8_t footer[] = {
        0xfa, 0xbc, 0xab, 0x09,
        0xd0, 0xc8, 0xd4, 0x66, 0xb1, 0x76, 0xfb, 0x83, 0x1c, 0xf7, 0x26, 0x7e, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xe8, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x5a, 0x8c, 0x6a,
        0xde, 0xf5, 0xd9, 0x7e, 0xec, 0xe9, 0x0c, 0xe3, 0x75, 0x8f, 0x29, 0x0b
    };
    writev(os, (char*)footer, std::size(footer));
}

void Document::write(const std::string& path)
{
    std::ofstream file;
    file.open(path, std::ios::out | std::ios::binary);
    if (file) {
        write(file);
    }
    else {
        throw std::runtime_error("Cannot write to file: \"" + path + "\"");
    }
}


FileVersion Document::getVersion()
{
    return m_version;
}


void Document::setVersion(FileVersion v)
{
    m_version = v;
}

Property* Document::createProperty()
{
    auto r = new Property();
    m_properties.push_back(PropertyPtr(r));
    return r;
}


Node* Document::createNode(const std::string& name)
{
    auto n = createChildNode(name);
    m_root_nodes.push_back(n);
    return n;
}
Node* Document::createChildNode(const std::string& name)
{
    auto r = new Node();
    r->m_document = this;
    r->setName(name);
    m_nodes.push_back(NodePtr(r));
    return r;
}

Node* Document::findNode(const char* name) const
{
    auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [name](const NodePtr& p) { return p->getName() == name; });
    return it != m_nodes.end() ? it->get() : nullptr;
}

span<Node*> Document::getRootNodes()
{
    return make_span(m_root_nodes);
}


Object* Document::createObject(ObjectType t)
{
    Object* r{};
    switch (t) {
    case ObjectType::Attribute: r = new Attribute(); break;
    case ObjectType::Model:     r = new Model(); break;
    case ObjectType::Geometry:  r = new Geometry(); break;
    case ObjectType::Deformer:  r = new Deformer(); break;
    case ObjectType::Pose:      r = new Pose(); break;
    case ObjectType::Material:  r = new Material(); break;
    default: break;
    }
    if (r) {
        r->m_document = this;
        m_objects.push_back(ObjectPtr(r));
    }
    return r;
}

template<class T>
T* Document::createObject()
{
    T* r = new T();
    r->m_document = this;
    m_objects.push_back(ObjectPtr(r));
    return r;
}
#define Body(T) template T* Document::createObject();
sfbxEachObjectType(Body)
#undef Body


Object* Document::findObject(int64 id)
{
    auto it = std::find_if(m_objects.begin(), m_objects.end(),
        [id](const ObjectPtr& p) { return p->getID() == id; });
    return it != m_objects.end() ? it->get() : nullptr;
}

span<Object*> Document::getRootObjects()
{
    return make_span(m_root_objects);
}

void Document::createHeaderExtention()
{
    auto header_extension = createNode(sfbxS_FBXHeaderExtension);
    header_extension->addPropertyNode(sfbxS_FBXHeaderVersion, (int32_t)1003);
    header_extension->addPropertyNode(sfbxS_FBXVersion, (int32_t)getVersion());
    header_extension->addPropertyNode(sfbxS_EncryptionType, (int32_t)0);

    {
        std::time_t t = std::time(nullptr);   // get time now
        std::tm* now = std::localtime(&t);

        auto timestamp = header_extension->createChild(sfbxS_CreationTimeStamp);
        timestamp->addPropertyNode("Version", 1000);
        timestamp->addPropertyNode("Year", now->tm_year);
        timestamp->addPropertyNode("Month", now->tm_mon);
        timestamp->addPropertyNode("Day", now->tm_mday);
        timestamp->addPropertyNode("Hour", now->tm_hour);
        timestamp->addPropertyNode("Minute", now->tm_min);
        timestamp->addPropertyNode("Second", now->tm_sec);
        timestamp->addPropertyNode("Millisecond", 0);
    }
    header_extension->addPropertyNode(sfbxS_Creator, "SmallFBX 1.0.0");
    {
        auto sceneInfo = header_extension->createChild(sfbxS_SceneInfo);
        sceneInfo->addProperty(PropertyType::String,
            RawVector<char>{'G', 'l', 'o', 'b', 'a', 'l', 'I', 'n', 'f', 'o', 0, 1, 'S', 'c', 'e', 'n', 'e', 'I', 'n', 'f', 'o'});
        sceneInfo->addProperty("UserData");
        sceneInfo->addPropertyNode("Type", "UserData");
        sceneInfo->addPropertyNode("Version", 100);
        {
            auto meta = sceneInfo->createChild(sfbxS_MetaData);
            meta->addPropertyNode("Version", 100);
            meta->addPropertyNode("Title", "");
            meta->addPropertyNode("Subject", "");
            meta->addPropertyNode("Author", "");
            meta->addPropertyNode("Keywords", "");
            meta->addPropertyNode("Revision", "");
            meta->addPropertyNode("Comment", "");
        }
        {
            auto properties = sceneInfo->createChild(sfbxS_Properties70);
            {
                //properties->createChild();
                auto p = properties->createChild(sfbxS_P);
                p->addProperty("DocumentUrl");
                p->addProperty("KString");
                p->addProperty("Url");
                p->addProperty("");
                p->addProperty("a.fbx");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty("SrcDocumentUrl");
                p->addProperty("KString");
                p->addProperty("Url");
                p->addProperty("");
                p->addProperty("a.fbx");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty("Original");
                p->addProperty("Compound");
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty("Original|ApplicationVendor");
                p->addProperty("KString");
                p->addProperty("");
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty("Original|ApplicationName");
                p->addProperty("KString");
                p->addProperty("");
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty("Original|ApplicationVersion");
                p->addProperty("KString");
                p->addProperty("");
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty("Original|DateTime_GMT");
                p->addProperty("DateTime");
                p->addProperty("");
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty("Original|FileName");
                p->addProperty("KString");
                p->addProperty("");
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty("LastSaved");
                p->addProperty("Compound");
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty("LastSaved|ApplicationVendor");
                p->addProperty("KString");
                p->addProperty("");
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty("LastSaved|ApplicationName");
                p->addProperty("KString");
                p->addProperty("");
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty("LastSaved|DateTime_GMT");
                p->addProperty("DateTime");
                p->addProperty("");
                p->addProperty("");
                p->addProperty("");
            }
        }
    }
}

void Document::constructNodes()
{
    createHeaderExtention();
    createNode(sfbxS_Objects);
    createNode(sfbxS_Connections);
    createNode(sfbxS_Takes);

    for (auto& o : m_objects)
        o->constructNodes();
}

} // namespace sfbx

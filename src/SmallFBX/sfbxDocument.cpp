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

bool Document::read(std::istream& is)
{
    char magic[23];
    readv(is, magic, 23);
    if (memcmp(magic, g_fbx_magic, 23) != 0) {
        // not a fbx file
        return false;
    }

    m_version = (FileVersion)read1<uint32_t>(is);

    uint64_t pos = 27; // magic (23) + version (4)
    for (;;) {
        auto node = createNode();
        pos += node->read(is, pos);
        if (node->isNull()) {
            eraseNode(node);
            break;
        }
    }

    if (auto objects = findNode(sfbxS_Objects)) {
        for (auto n : objects->getChildren()) {
            if (auto obj = createObject(GetFbxObjectType(n), GetFbxObjectSubType(n))) {
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
            else if (n->getName() == sfbxS_C && GetPropertyString(n, 0) == sfbxS_OP) {
                int64 cid = GetPropertyValue<int64>(n, 1);
                int64 pid = GetPropertyValue<int64>(n, 2);
                std::string name = GetPropertyString(n, 3); // todo
                Object* child = findObject(cid);
                Object* parent = findObject(pid);
                if (child && parent) {
                    parent->addChild(child);
                }
            }
            else {
                printf("sfbx::Document::read(): unrecognized connection type\n");
            }
        }
    }

    for (auto& obj : m_objects) {
        obj->constructObject();
        if (obj->getParents().empty())
            m_root_objects.push_back(obj.get());
    }
    return true;
}

bool Document::read(const std::string& path)
{
    std::ifstream file;
    file.open(path, std::ios::in | std::ios::binary);
    if (file)
        return read(file);
    return false;
}


bool Document::writeBinary(std::ostream& os)
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
    return true;
}

bool Document::writeBinary(const std::string& path)
{
    std::ofstream file;
    file.open(path, std::ios::out | std::ios::binary);
    if (file)
        return writeBinary(file);
    return false;
}

bool Document::writeAscii(std::ostream& output)
{
    auto data = toString();
    output.write(data.data(), data.size());
    return true;
}

bool Document::writeAscii(const std::string& path)
{
    std::ofstream file;
    file.open(path, std::ios::out | std::ios::binary);
    if (file)
        return writeAscii(file);
    return false;
}



FileVersion Document::getVersion()
{
    return m_version;
}


void Document::setVersion(FileVersion v)
{
    m_version = v;
}


Node* Document::createNode(const std::string& name)
{
    auto n = createChildNode(name);
    m_root_nodes.push_back(n);
    return n;
}
Node* Document::createChildNode(const std::string& name)
{
    auto n = new Node();
    n->m_document = this;
    n->setName(name);
    m_nodes.push_back(NodePtr(n));
    return n;
}

void Document::eraseNode(Node* n)
{
    {
        auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
            [n](const NodePtr& p) { return p.get() == n; });
        if (it != m_nodes.end())
            m_nodes.erase(it);
    }
    {
        auto it = std::find(m_root_nodes.begin(), m_root_nodes.end(), n);
        if (it != m_root_nodes.end())
            m_root_nodes.erase(it);
    }
}

Node* Document::findNode(const char* name) const
{
    auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [name](const NodePtr& p) { return p->getName() == name; });
    return it != m_nodes.end() ? it->get() : nullptr;
}

span<sfbx::NodePtr> Document::getAllNodes() { return make_span(m_nodes); }
span<Node*> Document::getRootNodes() { return make_span(m_root_nodes); }

Object* Document::createObject(ObjectType t, ObjectSubType s)
{
    Object* r{};
    switch (t) {
    case ObjectType::NodeAttribute: r = new NodeAttribute(); break;
    case ObjectType::Model:
        switch (s) {
        case ObjectSubType::Light: r = new Light(); break;
        case ObjectSubType::Camera: r = new Camera(); break;
        case ObjectSubType::Root: r = new Root(); break;
        case ObjectSubType::LimbNode: r = new LimbNode(); break;
        default: r = new Model(); break;
        }
        break;
    case ObjectType::Geometry:
        switch (s) {
        case ObjectSubType::Mesh: r = new Mesh(); break;
        case ObjectSubType::Shape: r = new Shape(); break;
        default: r = new Geometry(); break;
        }
        break;
    case ObjectType::Deformer:
        switch (s) {
        case ObjectSubType::Skin: r = new Skin(); break;
        case ObjectSubType::Cluster: r = new Cluster(); break;
        case ObjectSubType::BlendShape: r = new BlendShape(); break;
        case ObjectSubType::BlendShapeChannel: r = new BlendShapeChannel(); break;
        default: r = new Deformer(); break;
        }
        break;
    case ObjectType::Pose:
        switch (s) {
        case ObjectSubType::BindPose: r = new BindPose(); break;
        default: r = new Pose(); break;
        }
        break;
    case ObjectType::Material:  r = new Material(); break;
    case ObjectType::AnimationStack:    r = new AnimationStack(); break;
    case ObjectType::AnimationLayer:    r = new AnimationLayer(); break;
    case ObjectType::AnimationCurveNode:r = new AnimationCurveNode(); break;
    case ObjectType::AnimationCurve:    r = new AnimationCurve(); break;
    default: break;
    }

    if (r) {
        r->m_document = this;
        m_objects.push_back(ObjectPtr(r));
    }
    else {
        printf("sfbx::Document::createObject(): unrecongnized type \"%s\"\n", GetFbxObjectName(t));
    }
    return r;
}

template<class T>
T* Document::createObject(const std::string& name)
{
    T* r = new T();
    r->m_document = this;
    r->setName(name);
    m_objects.push_back(ObjectPtr(r));
    return r;
}
#define Body(T) template T* Document::createObject(const std::string& name);
sfbxEachObjectType(Body)
#undef Body


Object* Document::findObject(int64 id)
{
    auto it = std::find_if(m_objects.begin(), m_objects.end(),
        [id](const ObjectPtr& p) { return p->getID() == id; });
    return it != m_objects.end() ? it->get() : nullptr;
}

span<ObjectPtr> Document::getAllObjects() { return make_span(m_objects); }
span<Object*> Document::getRootObjects() { return make_span(m_root_objects); }

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
        timestamp->addPropertyNode(sfbxS_Version, 1000);
        timestamp->addPropertyNode(sfbxS_Year, now->tm_year);
        timestamp->addPropertyNode(sfbxS_Month, now->tm_mon);
        timestamp->addPropertyNode(sfbxS_Day, now->tm_mday);
        timestamp->addPropertyNode(sfbxS_Hour, now->tm_hour);
        timestamp->addPropertyNode(sfbxS_Minute, now->tm_min);
        timestamp->addPropertyNode(sfbxS_Second, now->tm_sec);
        timestamp->addPropertyNode(sfbxS_Millisecond, 0);
    }
    header_extension->addPropertyNode(sfbxS_Creator, "SmallFBX 1.0.0");
    {
        auto scene_info = header_extension->createChild(sfbxS_SceneInfo);
        scene_info->addProperty(PropertyType::String,
            RawVector<char>{'G', 'l', 'o', 'b', 'a', 'l', 'I', 'n', 'f', 'o', 0, 1, 'S', 'c', 'e', 'n', 'e', 'I', 'n', 'f', 'o'});
        scene_info->addProperty(sfbxS_UserData);
        scene_info->addPropertyNode(sfbxS_Type, sfbxS_UserData);
        scene_info->addPropertyNode(sfbxS_Version, 100);
        {
            auto meta = scene_info->createChild(sfbxS_MetaData);
            meta->addPropertyNode(sfbxS_Version, 100);
            meta->addPropertyNode(sfbxS_Title, "");
            meta->addPropertyNode(sfbxS_Subject, "");
            meta->addPropertyNode(sfbxS_Author, "");
            meta->addPropertyNode(sfbxS_Keywords, "");
            meta->addPropertyNode(sfbxS_Revision, "");
            meta->addPropertyNode(sfbxS_Comment, "");
        }
        {
            auto properties = scene_info->createChild(sfbxS_Properties70);
            {
                //properties->createChild();
                auto p = properties->createChild(sfbxS_P);
                p->addProperty(sfbxS_DocumentUrl);
                p->addProperty(sfbxS_KString);
                p->addProperty(sfbxS_Url);
                p->addProperty("");
                p->addProperty("a.fbx");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty(sfbxS_SrcDocumentUrl);
                p->addProperty(sfbxS_KString);
                p->addProperty(sfbxS_Url);
                p->addProperty("");
                p->addProperty("a.fbx");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty(sfbxS_Original);
                p->addProperty(sfbxS_Compound);
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty(sfbxS_OriginalApplicationVendor);
                p->addProperty(sfbxS_KString);
                p->addProperty("");
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty(sfbxS_OriginalApplicationName);
                p->addProperty(sfbxS_KString);
                p->addProperty("");
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty(sfbxS_OriginalApplicationVersion);
                p->addProperty(sfbxS_KString);
                p->addProperty("");
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty(sfbxS_OriginalDateTime_GMT);
                p->addProperty(sfbxS_DateTime);
                p->addProperty("");
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty(sfbxS_OriginalFileName);
                p->addProperty(sfbxS_KString);
                p->addProperty("");
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty(sfbxS_LastSaved);
                p->addProperty(sfbxS_Compound);
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty(sfbxS_LastSavedApplicationVendor);
                p->addProperty(sfbxS_KString);
                p->addProperty("");
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty(sfbxS_LastSavedApplicationName);
                p->addProperty(sfbxS_KString);
                p->addProperty("");
                p->addProperty("");
                p->addProperty("");
            }
            {
                auto p = properties->createChild(sfbxS_P);
                p->addProperty(sfbxS_LastSavedDateTime_GMT);
                p->addProperty(sfbxS_DateTime );
                p->addProperty("");
                p->addProperty("");
                p->addProperty("");
            }
        }
    }
}

// avoid template lambda because it requires C++20
template<class T>
int32 Document::countObject() const
{
    return (int32)count(m_objects, [](auto& p) { return as<T>(p.get()) != nullptr; });
}

void Document::createDefinitions()
{
    auto definitions = createNode(sfbxS_Definitions);

    auto add_object_type = [definitions](size_t n, const char* type) {
        if (n == 0)
            return;
        auto ot = definitions->createChild(sfbxS_ObjectType);
        ot->addProperty(sfbxS_NodeAttribute);
        ot->addPropertyNode(sfbxS_Count, (int32)n);
    };

    add_object_type(1, sfbxS_GlobalSettings);

    add_object_type(countObject<NodeAttribute>(), sfbxS_NodeAttribute);
    add_object_type(countObject<Model>(), sfbxS_Model);
    add_object_type(countObject<Geometry>(), sfbxS_Geometry);
    add_object_type(countObject<Deformer>(), sfbxS_Deformer);
    add_object_type(countObject<Pose>(), sfbxS_Pose);

    add_object_type(countObject<AnimationStack>(), sfbxS_AnimationStack);
    add_object_type(countObject<AnimationLayer>(), sfbxS_AnimationLayer);
    add_object_type(countObject<AnimationCurveNode>(), sfbxS_AnimationCurveNode);
    add_object_type(countObject<AnimationCurve>(), sfbxS_AnimationCurve);

    add_object_type(countObject<Material>(), sfbxS_Material);
}

void Document::constructNodes()
{
    createHeaderExtention();
    createNode(sfbxS_GlobalSettings);
    createNode(sfbxS_Documents);
    createNode(sfbxS_References);
    createDefinitions();
    createNode(sfbxS_Objects);
    createNode(sfbxS_Connections);
    createNode(sfbxS_Takes);

    for (auto& o : m_objects)
        o->constructNodes();
}

std::string Document::toString()
{
    char version[128];
    sprintf(version, "; FBX %d.%d.0 project file\n", (int)m_version / 1000 % 10, (int)m_version / 100 % 10);

    std::string s;
    s += version;
    s += "; ----------------------------------------------------\n\n";

    for (auto node : getRootNodes()) {
        // these nodes seem required only in binary format.
        if (node->getName() == sfbxS_FileId ||
            node->getName() == sfbxS_CreationTime ||
            node->getName() == sfbxS_Creator)
            continue;
        s += node->toString();
    }
    return s;
}

} // namespace sfbx

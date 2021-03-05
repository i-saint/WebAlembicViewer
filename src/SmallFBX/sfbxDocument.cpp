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
        sfbxPrint("sfbx::Document::read(): not a fbx file\n");
        return false;
    }
    m_version = (FileVersion)read1<uint32_t>(is);


    try {
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
                    sfbxPrint("sfbx::Document::read(): unrecognized connection type\n");
                }
            }
        }

        for (auto& obj : m_objects) {
            obj->constructObject();
            if (obj->getParents().empty())
                m_root_objects.push_back(obj.get());
        }
    }
    catch (const std::runtime_error& e) {
        sfbxPrint("sfbx::Document::read(): exception %s\n", e.what());
        return false;
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
    writev(os, g_fbx_magic, 23);
    write1(os, m_version);

    uint64_t pos = 27; // magic: 23, version: 4
    for (auto node : m_root_nodes)
        pos += node->write(os, pos);
    {
        Node null_node;
        null_node.m_document = this;
        pos += null_node.write(os, pos);
    }

    // footer

    const uint8_t footer_magic1[16] = { 0xfa, 0xbc, 0xab, 0x09, 0xd0, 0xc8, 0xd4, 0x66, 0xb1, 0x76, 0xfb, 0x83, 0x1c, 0xf7, 0x26, 0x7e };
    const uint8_t footer_magic2[16] = { 0xf8, 0x5a, 0x8c, 0x6a, 0xde, 0xf5, 0xd9, 0x7e, 0xec, 0xe9, 0x0c, 0xe3, 0x75, 0x8f, 0x29, 0x0b };

    writev(os, footer_magic1, std::size(footer_magic1));
    pos += std::size(footer_magic1);

    // add padding to 16 byte align
    uint64_t pad = 16 - (pos % 16);
    for (uint64_t i = 0; i < pad; ++i)
        write1(os, (int8)0);

    write1(os, (int32)0);
    write1(os, m_version);

    // 120 byte space
    for (uint64_t i = 0; i < 120; ++i)
        write1(os, (int8)0);

    writev(os, footer_magic2, std::size(footer_magic2));

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
        sfbxPrint("sfbx::Document::createObject(): unrecongnized type \"%s\"\n", GetFbxObjectName(t));
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


// avoid template lambda because it requires C++20
template<class T>
int32 Document::countObject() const
{
    return (int32)count(m_objects, [](auto& p) { return as<T>(p.get()) != nullptr; });
}

void Document::constructNodes()
{
    m_nodes.clear();
    m_root_nodes.clear();

    const char* smallfbx = "SmallFBX 1.0.0";

    // *** these must not be changed *** because it leads to CRC check failure.
    const char time_id[] = "1970-01-01 10:00:00:000";
    const uint8_t file_id[]{ 0x28, 0xb3, 0x2a, 0xeb, 0xb6, 0x24, 0xcc, 0xc2, 0xbf, 0xc8, 0xb0, 0x2a, 0xa9, 0x2b, 0xfc, 0xf1 };

    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);

    auto header_extension = createNode(sfbxS_FBXHeaderExtension);
    {
        header_extension->createChild(sfbxS_FBXHeaderVersion, (int32_t)1003);
        header_extension->createChild(sfbxS_FBXVersion, (int32_t)getVersion());
        header_extension->createChild(sfbxS_EncryptionType, (int32_t)0);
        {
            auto timestamp = header_extension->createChild(sfbxS_CreationTimeStamp);
            timestamp->createChild(sfbxS_Version, 1000);
            timestamp->createChild(sfbxS_Year, now->tm_year);
            timestamp->createChild(sfbxS_Month, now->tm_mon);
            timestamp->createChild(sfbxS_Day, now->tm_mday);
            timestamp->createChild(sfbxS_Hour, now->tm_hour);
            timestamp->createChild(sfbxS_Minute, now->tm_min);
            timestamp->createChild(sfbxS_Second, now->tm_sec);
            timestamp->createChild(sfbxS_Millisecond, 0);
        }
        header_extension->createChild(sfbxS_Creator, smallfbx);
        {
            auto other_flags = header_extension->createChild(sfbxS_OtherFlags);
            other_flags->createChild(sfbxS_TCDefinition, sfbxI_TCDefinition);
        }
        {
            auto scene_info = header_extension->createChild(sfbxS_SceneInfo, MakeObjectName(sfbxS_GlobalInfo, sfbxS_SceneInfo), sfbxS_UserData);
            scene_info->createChild(sfbxS_Type, sfbxS_UserData);
            scene_info->createChild(sfbxS_Version, 100);
            {
                auto meta = scene_info->createChild(sfbxS_MetaData);
                meta->createChild(sfbxS_Version, 100);
                meta->createChild(sfbxS_Title, "");
                meta->createChild(sfbxS_Subject, "");
                meta->createChild(sfbxS_Author, "");
                meta->createChild(sfbxS_Keywords, "");
                meta->createChild(sfbxS_Revision, "");
                meta->createChild(sfbxS_Comment, "");
            }
            {
                auto properties = scene_info->createChild(sfbxS_Properties70);
                properties->createChild(sfbxS_P, sfbxS_DocumentUrl, sfbxS_KString, sfbxS_Url, "", "a.fbx");
                properties->createChild(sfbxS_P, sfbxS_SrcDocumentUrl, sfbxS_KString, sfbxS_Url, "", "a.fbx");
                properties->createChild(sfbxS_P, sfbxS_Original, sfbxS_Compound, "", "");
                properties->createChild(sfbxS_P, sfbxS_OriginalApplicationVendor, sfbxS_KString, "", "", "");
                properties->createChild(sfbxS_P, sfbxS_OriginalApplicationName, sfbxS_KString, "", "", "");
                properties->createChild(sfbxS_P, sfbxS_OriginalApplicationVersion, sfbxS_KString, "", "", "");
                properties->createChild(sfbxS_P, sfbxS_OriginalDateTime_GMT, sfbxS_DateTime, "", "", "");
                properties->createChild(sfbxS_P, sfbxS_OriginalFileName, sfbxS_KString, "", "", "");
                properties->createChild(sfbxS_P, sfbxS_LastSaved, sfbxS_Compound, "", "");
                properties->createChild(sfbxS_P, sfbxS_LastSavedApplicationVendor, sfbxS_KString, "", "", "");
                properties->createChild(sfbxS_P, sfbxS_LastSavedApplicationName, sfbxS_KString, "", "", "");
                properties->createChild(sfbxS_P, sfbxS_LastSavedApplicationVersion, sfbxS_KString, "", "", "");
                properties->createChild(sfbxS_P, sfbxS_LastSavedDateTime_GMT, sfbxS_DateTime, "", "", "");
            }
        }
    }

    createNode(sfbxS_FileId)->addProperty(make_span(file_id));
    createNode(sfbxS_CreationTime)->addProperties(time_id);
    createNode(sfbxS_Creator)->addProperties(smallfbx);

    auto global_settings = createNode(sfbxS_GlobalSettings);
    {
        global_settings->createChild(sfbxS_Version, sfbxI_GlobalSettingsVersion);
        auto prop = global_settings->createChild(sfbxS_Properties70);
        prop->createChild(sfbxS_P, "UpAxis", "int", "Integer", "", 1);
        prop->createChild(sfbxS_P, "UpAxisSign", "int", "Integer", "", 1);
        prop->createChild(sfbxS_P, "FrontAxis", "int", "Integer", "", 2);
        prop->createChild(sfbxS_P, "FrontAxisSign", "int", "Integer", "", 1);
        prop->createChild(sfbxS_P, "CoordAxis", "int", "Integer", "", 0);
        prop->createChild(sfbxS_P, "CoordAxisSign", "int", "Integer", "", 1);
        prop->createChild(sfbxS_P, "OriginalUpAxis", "int", "Integer", "", -1);
        prop->createChild(sfbxS_P, "OriginalUpAxisSign", "int", "Integer", "", 1);
        prop->createChild(sfbxS_P, "UnitScaleFactor", "double", "Number", "", 1.000000);
        prop->createChild(sfbxS_P, "OriginalUnitScaleFactor", "double", "Number", "", 1.000000);
        prop->createChild(sfbxS_P, "AmbientColor", "ColorRGB", "Color", "", 0.000000, 0.000000, 0.000000);
        prop->createChild(sfbxS_P, "DefaultCamera", "KString", "", "", "Producer Perspective");
        prop->createChild(sfbxS_P, "TimeMode", "enum", "", "", 0);
        prop->createChild(sfbxS_P, "TimeProtocol", "enum", "", "", 2);
        prop->createChild(sfbxS_P, "SnapOnFrameMode", "enum", "", "", 0);
        prop->createChild(sfbxS_P, "TimeSpanStart", "KTime", "Time", "", (int64)0);
        prop->createChild(sfbxS_P, "TimeSpanStop", "KTime", "Time", "", (int64)sfbxI_TicksPerSecond);
        prop->createChild(sfbxS_P, "CustomFrameRate", "double", "Number", "", -1.000000);
        prop->createChild(sfbxS_P, "TimeMarker", "Compound", "", "");
        prop->createChild(sfbxS_P, "CurrentTimeMarker", "int", "Integer", "", -1);
    }

    auto documents = createNode(sfbxS_Documents);
    {
        documents->createChild(sfbxS_Count, (int32)1);
        auto doc = documents->createChild(sfbxS_Document);
        {
            doc->addProperties((int64)this, "My Scene", "Scene");

            auto prop = doc->createChild(sfbxS_Properties70);
            prop->createChild(sfbxS_P, "SourceObject", "object", "", "");
            prop->createChild(sfbxS_P, "ActiveAnimStackName", "KString", "", "", "");

            doc->createChild(sfbxS_RootNode, 0);
        }
    }

    auto references = createNode(sfbxS_References);

    auto definitions = createNode(sfbxS_Definitions);

    createNode(sfbxS_Objects);
    createNode(sfbxS_Connections);

    createNode(sfbxS_Takes)->createChild(sfbxS_Current, "");

    // index based loop because m_objects maybe push_backed in the loop
    for (size_t i = 0; i < m_objects.size(); ++i)
        m_objects[i]->constructNodes();

    {
        auto add_object_type = [definitions](size_t n, const char* type) -> Node* {
            if (n == 0)
                return nullptr;
            auto ot = definitions->createChild(sfbxS_ObjectType);
            ot->addProperty(type);
            ot->createChild(sfbxS_Count, (int32)n);
            return ot;
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

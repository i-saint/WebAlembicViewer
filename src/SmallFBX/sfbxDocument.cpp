#include "pch.h"
#include "sfbxDocument.h"
#include "sfbxUtil.h"

namespace sfbx {

Document::Document()
{
    m_version = 7400;
}

void Document::read(const std::string& fname)
{
    std::ifstream file;
    file.open(fname, std::ios::in | std::ios::binary);
    if (file.is_open()) {
        read(file);
    }
    else {
        throw std::runtime_error("Cannot read from file: \"" + fname + "\"");
    }
    file.close();
}

void Document::write(const std::string& fname)
{
    std::ofstream file;
    file.open(fname, std::ios::out | std::ios::binary);
    if (file.is_open()) {
        write(file);
    } else {
        throw std::runtime_error("Cannot write to file: \"" + fname + "\"");
    }
    file.close();
}

static bool checkMagic(std::istream& is)
{
    const char magic[] = "Kaydara FBX Binary  ";
    for (char c : magic) {
        if (read1<char>(is) != c)
            return false;
    }
    if (read1<uint8_t>(is) != 0x00) return false;
    if (read1<uint8_t>(is) != 0x1A) return false;
    if (read1<uint8_t>(is) != 0x00) return false;
    return true;
}

void Document::read(std::istream &is)
{
    is >> std::noskipws;
    if (!checkMagic(is))
        throw std::runtime_error("Not a FBX file");

    uint32_t version = read1<uint32_t>(is);

    uint32_t maxVersion = 7400;
    if(version > maxVersion) throw "Unsupported FBX version "+std::to_string(version)
                            + " latest supported version is "+std::to_string(maxVersion);

    uint32_t start_offset = 27; // magic: 21+2, version: 4
    do {
        Node node;
        start_offset += node.read(is, start_offset);
        if (node.isNull())
            break;
        m_nodes.push_back(node);
    } while (true);
}

void Document::write(std::ostream& os)
{
    write1(os, "Kaydara FBX Binary  ");
    write1(os, (uint8_t)0);
    write1(os, (uint8_t)0x1A);
    write1(os, (uint8_t)0);
    write1(os, m_version);

    uint32_t offset = 27; // magic: 21+2, version: 4
    for (auto& node : m_nodes)
        offset += node.write(os, offset);

    Node null_node;
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

void Document::createBasicStructure()
{
    auto addPropertyNode = [](Node& node, const std::string& name, const auto& value) {
        Node c(name);
        c.addProperty(value);
        node.addChild(std::move(c));
    };

    Node headerExtension("FBXHeaderExtension");
    addPropertyNode(headerExtension, "FBXHeaderVersion", (int32_t)1003);
    addPropertyNode(headerExtension, "FBXVersion", (int32_t)getVersion());
    addPropertyNode(headerExtension, "EncryptionType", (int32_t)0);
    {
        Node timestamp("CreationTimeStamp");
        addPropertyNode(timestamp, "Version", (int32_t)1000);
        addPropertyNode(timestamp, "Year", (int32_t)2017);
        addPropertyNode(timestamp, "Month", (int32_t)5);
        addPropertyNode(timestamp, "Day", (int32_t)2);
        addPropertyNode(timestamp, "Hour", (int32_t)14);
        addPropertyNode(timestamp, "Minute", (int32_t)11);
        addPropertyNode(timestamp, "Second", (int32_t)46);
        addPropertyNode(timestamp, "Millisecond", (int32_t)917);
        headerExtension.addChild(std::move(timestamp));
    }
    addPropertyNode(headerExtension, "Creator", std::string("SmallFBX 1.0.0"));
    {
        Node sceneInfo("SceneInfo");
        sceneInfo.addProperty(PropertyType::String,
            std::vector<char>{'G', 'l', 'o', 'b', 'a', 'l', 'I', 'n', 'f', 'o', 0, 1, 'S', 'c', 'e', 'n', 'e', 'I', 'n', 'f', 'o'});
        sceneInfo.addProperty("UserData");
        addPropertyNode(sceneInfo, "Type", "UserData");
        addPropertyNode(sceneInfo, "Version", 100);
        {
            Node metadata("MetaData");
            addPropertyNode(metadata, "Version", 100);
            addPropertyNode(metadata, "Title", "");
            addPropertyNode(metadata, "Subject", "");
            addPropertyNode(metadata, "Author", "");
            addPropertyNode(metadata, "Keywords", "");
            addPropertyNode(metadata, "Revision", "");
            addPropertyNode(metadata, "Comment", "");
            sceneInfo.addChild(std::move(metadata));
        }
        {
            Node properties("Properties70");
            {
                Node p("P");
                p.addProperty("DocumentUrl");
                p.addProperty("KString");
                p.addProperty("Url");
                p.addProperty("");
                p.addProperty("/foobar.fbx");
                properties.addChild(std::move(p));
            }
            {
                Node p("P");
                p.addProperty("SrcDocumentUrl");
                p.addProperty("KString");
                p.addProperty("Url");
                p.addProperty("");
                p.addProperty("/foobar.fbx");
                properties.addChild(std::move(p));
            }
            {
                Node p("P");
                p.addProperty("Original");
                p.addProperty("Compound");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(std::move(p));
            }
            {
                Node p("P");
                p.addProperty("Original|ApplicationVendor");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("i-saint");
                properties.addChild(std::move(p));
            }
            {
                Node p("P");
                p.addProperty("Original|ApplicationName");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("SmallFBX");
                properties.addChild(std::move(p));
            }
            {
                Node p("P");
                p.addProperty("Original|ApplicationVersion");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("1.0.0");
                properties.addChild(std::move(p));
            }
            {
                Node p("P");
                p.addProperty("Original|DateTime_GMT");
                p.addProperty("DateTime");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("01/01/1970 00:00:00.000");
                properties.addChild(std::move(p));
            }
            {
                Node p("P");
                p.addProperty("Original|FileName");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("/foobar.fbx");
                properties.addChild(std::move(p));
            }
            {
                Node p("P");
                p.addProperty("LastSaved");
                p.addProperty("Compound");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(std::move(p));
            }
            {
                Node p("P");
                p.addProperty("LastSaved|ApplicationVendor");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("Blender Foundation");
                properties.addChild(std::move(p));
            }
            {
                Node p("P");
                p.addProperty("LastSaved|ApplicationName");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("Blender (stable FBX IO)");
                properties.addChild(std::move(p));
            }
            {
                Node p("P");
                p.addProperty("LastSaved|DateTime_GMT");
                p.addProperty("DateTime");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("01/01/1970 00:00:00.000");
                properties.addChild(std::move(p));
            }
            sceneInfo.addChild(std::move(properties));
        }
        headerExtension.addChild(std::move(sceneInfo));
    }
    m_nodes.push_back(std::move(headerExtension));


}

std::uint32_t Document::getVersion()
{
    return m_version;
}

} // namespace sfbx

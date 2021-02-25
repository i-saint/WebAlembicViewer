#include "pch.h"
#include "fbxDocument.h"
#include "fbxUtil.h"

namespace fbx {

FBXDocument::FBXDocument()
{
    m_version = 7400;
}

void FBXDocument::read(const std::string& fname)
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

void FBXDocument::write(const std::string& fname)
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

void FBXDocument::read(std::istream &is)
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
        FBXNode node;
        start_offset += node.read(is, start_offset);
        if (node.isNull())
            break;
        m_nodes.push_back(node);
    } while (true);
}

void FBXDocument::write(std::ostream& os)
{
    write_s(os, "Kaydara FBX Binary  ");
    write1(os, (uint8_t)0);
    write1(os, (uint8_t)0x1A);
    write1(os, (uint8_t)0);
    write1(os, m_version);

    uint32_t offset = 27; // magic: 21+2, version: 4
    for (auto& node : m_nodes) {
        offset += node.write(os, offset);
    }
    FBXNode nullNode;
    offset += nullNode.write(os, offset);

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
    os.write((char*)footer, std::size(footer));
}

void FBXDocument::createBasicStructure()
{
    auto addPropertyNode = [](FBXNode& node, const std::string& name, const auto& value) {
        FBXNode c(name);
        c.addProperty(value);
        node.addChild(c);
    };

    FBXNode headerExtension("FBXHeaderExtension");
    addPropertyNode(headerExtension, "FBXHeaderVersion", (int32_t)1003);
    addPropertyNode(headerExtension, "FBXVersion", (int32_t)getVersion());
    addPropertyNode(headerExtension, "EncryptionType", (int32_t)0);
    {
        FBXNode creationTimeStamp("CreationTimeStamp");
        addPropertyNode(creationTimeStamp, "Version", (int32_t)1000);
        addPropertyNode(creationTimeStamp, "Year", (int32_t)2017);
        addPropertyNode(creationTimeStamp, "Month", (int32_t)5);
        addPropertyNode(creationTimeStamp, "Day", (int32_t)2);
        addPropertyNode(creationTimeStamp, "Hour", (int32_t)14);
        addPropertyNode(creationTimeStamp, "Minute", (int32_t)11);
        addPropertyNode(creationTimeStamp, "Second", (int32_t)46);
        addPropertyNode(creationTimeStamp, "Millisecond", (int32_t)917);
        headerExtension.addChild(creationTimeStamp);
    }
    addPropertyNode(headerExtension, "Creator", std::string("SmallFBX 1.0.0"));
    {
        FBXNode sceneInfo("SceneInfo");
        sceneInfo.addProperty(std::vector<char>{'G', 'l', 'o', 'b', 'a', 'l', 'I', 'n', 'f', 'o', 0, 1, 'S', 'c', 'e', 'n', 'e', 'I', 'n', 'f', 'o'}, 'S');
        sceneInfo.addProperty("UserData");
        addPropertyNode(sceneInfo, "Type", "UserData");
        addPropertyNode(sceneInfo, "Version", 100);
        {
            FBXNode metadata("MetaData");
            addPropertyNode(metadata, "Version", 100);
            addPropertyNode(metadata, "Title", "");
            addPropertyNode(metadata, "Subject", "");
            addPropertyNode(metadata, "Author", "");
            addPropertyNode(metadata, "Keywords", "");
            addPropertyNode(metadata, "Revision", "");
            addPropertyNode(metadata, "Comment", "");
            sceneInfo.addChild(metadata);
        }
        {
            FBXNode properties("Properties70");
            {
                FBXNode p("P");
                p.addProperty("DocumentUrl");
                p.addProperty("KString");
                p.addProperty("Url");
                p.addProperty("");
                p.addProperty("/foobar.fbx");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("SrcDocumentUrl");
                p.addProperty("KString");
                p.addProperty("Url");
                p.addProperty("");
                p.addProperty("/foobar.fbx");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Original");
                p.addProperty("Compound");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Original|ApplicationVendor");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("i-saint");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Original|ApplicationName");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("SmallFBX");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Original|ApplicationVersion");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("1.0.0");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Original|DateTime_GMT");
                p.addProperty("DateTime");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("01/01/1970 00:00:00.000");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Original|FileName");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("/foobar.fbx");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("LastSaved");
                p.addProperty("Compound");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("LastSaved|ApplicationVendor");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("Blender Foundation");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("LastSaved|ApplicationName");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("Blender (stable FBX IO)");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("LastSaved|DateTime_GMT");
                p.addProperty("DateTime");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("01/01/1970 00:00:00.000");
                properties.addChild(p);
            }
            sceneInfo.addChild(properties);
        }
        headerExtension.addChild(sceneInfo);
    }
    m_nodes.push_back(headerExtension);


}

std::uint32_t FBXDocument::getVersion()
{
    return m_version;
}

} // namespace fbx

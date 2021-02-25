#pragma once
#include "fbxProperty.h"

namespace fbx {

class FBXNode
{
public:
    FBXNode(std::string name = "");

    std::uint32_t read(std::istream &input, uint32_t start_offset);
    std::uint32_t write(std::ostream &output, uint32_t start_offset);
    bool isNull();

    void addProperty(const FBXProperty& v);
    void addProperty(const std::vector<char>& v, uint8_t t) { addProperty(FBXProperty(v, t)); }
    void addProperty(const char* v) { addProperty(FBXProperty(v)); }
    template<class T> void addProperty(const T& v) { addProperty(FBXProperty(v)); }
    template<class T> void addProperty(const std::vector<T>& v) { addProperty(FBXProperty(v)); }

    void addChild(FBXNode child);
    uint32_t getBytes() const;

    const std::string& getName() const;
    const std::vector<FBXProperty>& getProperties();
    const std::vector<FBXNode>& getChildren();

private:
    std::string m_name;
    std::vector<FBXProperty> m_properties;
    std::vector<FBXNode> m_children;
};

} // namespace fbx

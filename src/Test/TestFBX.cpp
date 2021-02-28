#include "pch.h"
#include "Test.h"
#include "SmallFBX/SmallFBX.h"

static void PrintObject(sfbx::Object* obj, int depth = 0)
{
    // indent
    for (int i = 0; i < depth; ++i)
        testPrint("  ");

    testPrint("\"%s\" [0x%llx] (%s : %s)\n",
        obj->getName().c_str(),
        obj->getID(),
        sfbx::GetFbxObjectName(obj->getType()),
        sfbx::GetFbxObjectSubName(obj->getSubType()));

    if (obj->getType() == sfbx::ObjectType::Deformer && obj->getSubType() == sfbx::ObjectSubType::Skin) {
        auto skin = dynamic_cast<sfbx::Deformer*>(obj);
        auto weights_variable = skin->skinMakeWeightsVariable();
        auto weights_4 = skin->skinMakeWeightsFixed(4);
        testPrint("");
    }

    for (auto child : obj->getChildren())
        PrintObject(child, depth + 1);
}

testCase(fbxRead)
{
    std::string path;
    test::GetArg("path", path);
    if (path.empty())
        return;

    sfbx::DocumentPtr doc = sfbx::MakeDocument();
    doc->read(path);

    for (auto obj : doc->getRootObjects())
        PrintObject(obj);
}

testCase(fbxWrite)
{
    {
        sfbx::DocumentPtr doc = sfbx::MakeDocument();

        auto model = doc->createObject<sfbx::Model>();
        model->setName("model");
        // todo

        doc->write("test.fbx");
    }

    {
        sfbx::DocumentPtr doc = sfbx::MakeDocument();
        doc->read("test.fbx");
        for (auto obj : doc->getRootObjects())
            PrintObject(obj);
    }
}

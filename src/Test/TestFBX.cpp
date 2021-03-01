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

    if (auto skin = sfbx::as<sfbx::Skin>(obj)) {
        auto weights_v = skin->getJointWeightsVariable();
        auto weights_4 = skin->getJointWeightsFixed(4);
        auto matrices = skin->getJointMatrices();
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

    testPrint("Nodes:\n");
    testPuts(doc->toString().c_str());

    testPrint("Objects:\n");
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

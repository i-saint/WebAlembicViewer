#include "pch.h"
#include "Test.h"
#include "SmallFBX/SmallFBX.h"

static void PrintObject(sfbx::Object* obj, int depth = 0)
{
    // indent
    for (int i = 0; i < depth; ++i)
        testPrint("  ");

    testPrint("\"%s\" (%s : %s)\n",
        obj->getName().c_str(),
        sfbx::GetFbxObjectName(obj->getType()),
        sfbx::GetFbxObjectSubName(obj->getSubType()));

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

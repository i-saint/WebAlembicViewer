#include "pch.h"
#include "Test.h"
#include "SmallFBX.h"

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
    else if (auto anim = sfbx::as<sfbx::AnimationLayer>(obj)) {
        if (auto pos = anim->getPosition()) {
            float start = pos->getStartTime();
            float end = pos->getEndTime();
            for (float t = start; t <= end; t += 0.033334f) {
                auto v = pos->evaluate3(t);
                testPrint("");
            }
        }
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
    if (doc->read(path)) {
        testPrint("Nodes:\n");
        testPuts(doc->toString().c_str());

        testPrint("Objects:\n");
        for (auto obj : doc->getRootObjects())
            PrintObject(obj);

        doc->writeAscii("out_ascii.fbx");
    }

}

testCase(fbxWrite)
{
    {
        sfbx::DocumentPtr doc = sfbx::MakeDocument();

        auto model = doc->createObject<sfbx::Model>();
        model->setName("model");
        // todo

        doc->writeBinary("test.fbx");
    }

    {
        sfbx::DocumentPtr doc = sfbx::MakeDocument();
        doc->read("test.fbx");
        for (auto obj : doc->getRootObjects())
            PrintObject(obj);
    }
}

testCase(fbxAnimationCurve)
{
    sfbx::DocumentPtr doc = sfbx::MakeDocument();
    auto curve = doc->createObject<sfbx::AnimationCurve>("TestCurve");

    sfbx::RawVector<float> times{ 0.0f, 1.0f, 2.0f };
    sfbx::RawVector<float> values{ 0.0f, 100.0f, 400.0f };
    curve->setTimes(times);
    curve->setValues(values);

    for (float t = -0.5f; t < 2.5f; t += 0.1f) {
        printf("time: %f, value: %f\n", t, curve->evaluate(t));
    }
}

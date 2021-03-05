#include "pch.h"
#include "Test.h"
#include "SmallFBX.h"

using sfbx::as;
using sfbx::span;
using sfbx::make_span;
using sfbx::RawVector;
using sfbx::float3;

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

    if (auto skin = as<sfbx::Skin>(obj)) {
        auto weights_v = skin->getJointWeightsVariable();
        auto weights_4 = skin->getJointWeightsFixed(4);
        auto matrices = skin->getJointMatrices();
        testPrint("");
    }
    else if (auto anim = as<sfbx::AnimationLayer>(obj)) {
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

        doc->writeBinary("out_b.fbx");
        doc->writeAscii("out_a.fbx");
    }

}

testCase(fbxWrite)
{
    {
        sfbx::DocumentPtr doc = sfbx::MakeDocument();

        auto model = doc->createObject<sfbx::Model>("mesh");
        model->setPosition({ 0.0f, 0.0f, 0.0f });
        model->setRotation({ 0.0f, 0.0f, 0.0f });
        model->setScale({ 1.0f, 1.0f, 1.0f });

        float s = 10.0f;
        auto mesh = model->createChild<sfbx::Mesh>();

        {
            RawVector<int> counts{ 4, 4, 4, 4 };
            RawVector<int> indices{
                0, 1, 3, 2,
                2, 3, 5, 4,
                4, 5, 7, 6,
                6, 7, 9, 8,
            };

            RawVector<float3> points{
                {-s * 0.5f, s * 0, 0}, {s * 0.5f, s * 0, 0},
                {-s * 0.5f, s * 1, 0}, {s * 0.5f, s * 1, 0},
                {-s * 0.5f, s * 2, 0}, {s * 0.5f, s * 2, 0},
                {-s * 0.5f, s * 3, 0}, {s * 0.5f, s * 3, 0},
                {-s * 0.5f, s * 4, 0}, {s * 0.5f, s * 4, 0},
            };
            mesh->setCounts(counts);
            mesh->setIndices(indices);
            mesh->setPoints(points);

            {
                sfbx::LayerElementF3 normals;
                normals.data = {
                    { 0, 0, -1}, { 0, 0, -1},
                    { 0, 0, -1}, { 0, 0, -1},
                    { 0, 0, -1}, { 0, 0, -1},
                    { 0, 0, -1}, { 0, 0, -1},
                    { 0, 0, -1}, { 0, 0, -1},
                };
                normals.indices = indices;
                mesh->addNormalLayer(std::move(normals));
            }
            {
                sfbx::LayerElementF2 uv;
                uv.data = {
                    { 0, 0.00f }, { 1, 0.00f },
                    { 0, 0.25f }, { 1, 0.25f },
                    { 0, 0.50f }, { 1, 0.50f },
                    { 0, 0.75f }, { 1, 0.75f },
                    { 0, 1.00f }, { 1, 1.00f },
                };
                uv.indices = indices;
                mesh->addUVLayer(std::move(uv));
            }
            {
                sfbx::LayerElementF4 colors;
                colors.data = {
                    { 1, 0, 0, 1}, { 0, 1, 0, 1},
                    { 0, 0, 1, 1}, { 0, 0, 0, 1},
                    { 1, 0, 0, 1}, { 0, 1, 0, 1},
                    { 0, 0, 1, 1}, { 0, 0, 0, 1},
                    { 0, 0, 1, 1}, { 0, 0, 0, 1},
                };
                colors.indices = indices;
                mesh->addColorLayer(std::move(colors));

            }
        }

        sfbx::Model* bones[5]{};
        bones[0] = doc->createObject<sfbx::Root>("joint1");
        bones[1] = bones[0]->createChild<sfbx::LimbNode>("joint2");
        bones[2] = bones[1]->createChild<sfbx::LimbNode>("joint3");
        bones[3] = bones[2]->createChild<sfbx::LimbNode>("joint4");
        bones[4] = bones[3]->createChild<sfbx::LimbNode>("joint5");

        sfbx::BindPose* bindpose = doc->createObject<sfbx::BindPose>();
        for (int i = 0; i < 5; ++i) {
            bones[i]->setPosition({ 0.0f, i == 0 ? 0.0f : s, 0.0f });
            bindpose->addPoseData(bones[i], bones[i]->getGlobalMatrix());
        }


        auto skin = mesh->createChild<sfbx::Skin>();
        skin->setName("");

        sfbx::Cluster* clusters[5]{};
        for (int i = 0; i < 5; ++i) {
            clusters[i] = skin->createChild<sfbx::Cluster>();
            clusters[i]->addChild(bones[i]);

            int indices[2]{ i * 2 + 0, i * 2 + 1 };
            float weights[2]{ 1.0f, 1.0f };
            clusters[i]->setIndices(make_span(indices));
            clusters[i]->setWeights(make_span(weights));
        }


        doc->constructNodes();
        doc->writeBinary("test_b.fbx");
        doc->writeAscii("test_a.fbx");
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

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
        obj->getName().data(),
        obj->getID(),
        sfbx::GetObjectClassName(obj->getClass()),
        sfbx::GetObjectSubClassName(obj->getSubClass()));

    // for test
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

    if (!as<sfbx::Cluster>(obj)) {
        // avoid printing children of Cluster because it may result in a nearly endless list

        for (auto child : obj->getChildren())
            PrintObject(child, depth + 1);

    }
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
        auto root = doc->getRootModel();

        auto node = root->createChild<sfbx::Mesh>("mesh");
        node->setPosition({ 0.0f, 0.0f, 0.0f });
        node->setRotation({ 0.0f, 0.0f, 0.0f });
        node->setScale({ 1.0f, 1.0f, 1.0f });

        float s = 10.0f;
        auto mesh = node->getGeometry();

        {
            // counts & indices & points
            int counts[]{
                4, 4, 4, 4,
            };
            int indices[]{
                0, 1, 3, 2,
                2, 3, 5, 4,
                4, 5, 7, 6,
                6, 7, 9, 8,
            };
            float3 points[]{
                {-s * 0.5f, s * 0, 0}, {s * 0.5f, s * 0, 0},
                {-s * 0.5f, s * 1, 0}, {s * 0.5f, s * 1, 0},
                {-s * 0.5f, s * 2, 0}, {s * 0.5f, s * 2, 0},
                {-s * 0.5f, s * 3, 0}, {s * 0.5f, s * 3, 0},
                {-s * 0.5f, s * 4, 0}, {s * 0.5f, s * 4, 0},
            };
            mesh->setCounts(counts);
            mesh->setIndices(indices);
            mesh->setPoints(points);

            // normals
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

            // uv
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

            // colors
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

        // blend shape
        {
            int indices[]{
                6, 7, 8, 9,
            };
            float3 delta_points[]{
                {-s, 0, 0}, {s, 0, 0},
                {-s, 0, 0}, {s, 0, 0},
            };

            sfbx::BlendShape* blendshape = mesh->createBlendshape();
            sfbx::BlendShapeChannel* channel = blendshape->createChannel("shape");
            sfbx::Shape* shape = channel->createShape("shape", 1.0f);
            shape->setIndices(indices);
            shape->setDeltaPoints(delta_points);

            // verify
            RawVector<float3> points;
            for (float w = -0.1f; w < 1.1f; w+=0.1f) {
                points = mesh->getPoints();
                channel->setWeight(w);
                blendshape->deformPoints(points);
                testPrint("");
            }
        }

        // joints & skin
        {
            sfbx::Model* joints[5]{};
            joints[0] = root->createChild<sfbx::Root>("joint1");
            joints[1] = joints[0]->createChild<sfbx::LimbNode>("joint2");
            joints[2] = joints[1]->createChild<sfbx::LimbNode>("joint3");
            joints[3] = joints[2]->createChild<sfbx::LimbNode>("joint4");
            joints[4] = joints[3]->createChild<sfbx::LimbNode>("joint5");
            for (int i = 1; i < 5; ++i)
                joints[i]->setPosition({ 0, s, 0 });

            sfbx::Skin* skin = mesh->createSkin();
            for (int i = 0; i < 5; ++i) {
                sfbx::Cluster* cluster = skin->createCluster(joints[i]);
                int indices[2]{ i * 2 + 0, i * 2 + 1 };
                float weights[2]{ 1.0f, 1.0f };
                cluster->setIndices(make_span(indices));
                cluster->setWeights(make_span(weights));
                cluster->setBindMatrix(joints[i]->getGlobalMatrix());
            }
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

    float times[]{ 0.0f, 1.0f, 2.0f };
    float values[]{ 0.0f, 100.0f, 400.0f };
    curve->setTimes(times);
    curve->setValues(values);

    for (float t = -0.5f; t < 2.5f; t += 0.1f) {
        printf("time: %f, value: %f\n", t, curve->evaluate(t));
    }
}

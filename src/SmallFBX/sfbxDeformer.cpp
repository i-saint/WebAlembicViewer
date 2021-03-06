#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxObject.h"
#include "sfbxDocument.h"

namespace sfbx {

ObjectClass Deformer::getClass() const { return ObjectClass::Deformer; }

string_view SubDeformer::getClassName() const { return sfbxS_SubDeformer; }


ObjectSubClass Skin::getSubClass() const { return ObjectSubClass::Skin; }

void Skin::constructObject()
{
    super::constructObject();
}

void Skin::constructNodes()
{
    super::constructNodes();

    auto n = getNode();
    n->createChild(sfbxS_Version, sfbxI_SkinVersion);
    n->createChild(sfbxS_Link_DeformAcuracy, (float64)50.0);

}

void Skin::addParent(Object* v)
{
    super::addParent(v);
    if (auto mesh = as<GeomMesh>(v))
        m_mesh = mesh;
}

void Skin::addChild(Object* v)
{
    super::addChild(v);
    if (auto cluster = as<Cluster>(v))
        m_clusters.push_back(cluster);
}

GeomMesh* Skin::getMesh() const { return m_mesh; }
span<Cluster*> Skin::getClusters() const { return make_span(m_clusters); }

JointWeights Skin::getJointWeightsVariable()
{
    JointWeights ret;
    auto mesh = as<GeomMesh>(getParent());
    if (!mesh)
        return ret;

    size_t cclusters = m_clusters.size();
    size_t cpoints = mesh->getPoints().size();
    size_t total_weights = 0;

    // setup counts
    ret.counts.resize(cpoints, 0);
    for (auto cluster : m_clusters) {
        auto indices = cluster->getIndices();
        for (int vi : indices)
            ret.counts[vi]++;
        total_weights += indices.size();
    }

    // setup offsets
    ret.offsets.resize(cpoints);
    size_t offset = 0;
    int max_joints_per_vertex = 0;
    for (size_t pi = 0; pi < cpoints; ++pi) {
        ret.offsets[pi] = offset;
        int c = ret.counts[pi];
        offset += c;
        max_joints_per_vertex = std::max(max_joints_per_vertex, c);
    }
    ret.max_joints_per_vertex = max_joints_per_vertex;

    // setup weights
    ret.counts.zeroclear(); // clear to recount
    ret.weights.resize(total_weights);
    for (size_t ci = 0; ci < cclusters; ++ci) {
        auto cluster = m_clusters[ci];
        auto indices = cluster->getIndices();
        auto weights = cluster->getWeights();
        size_t cweights = weights.size();
        for (size_t wi = 0; wi < cweights; ++wi) {
            int vi = indices[wi];
            float weight = weights[wi];
            int pos = ret.offsets[vi] + ret.counts[vi]++;
            ret.weights[pos] = { (int)ci, weight };
        }
    }
    return ret;
}

JointWeights Skin::getJointWeightsFixed(int joints_per_vertex)
{
    JointWeights tmp = getJointWeightsVariable();
    if (tmp.weights.empty())
        return tmp;

    JointWeights ret;
    size_t cpoints = tmp.counts.size();
    ret.max_joints_per_vertex = std::min(joints_per_vertex, tmp.max_joints_per_vertex);
    ret.counts.resize(cpoints);
    ret.offsets.resize(cpoints);
    ret.weights.resize(cpoints * joints_per_vertex);
    ret.weights.zeroclear();

    auto normalize_weights = [](span<JointWeight> weights) {
        float total = 0.0f;
        for (auto& w : weights)
            total += w.weight;

        if (total != 0.0f) {
            float rcp_total = 1.0f / total;
            for (auto& w : weights)
                w.weight *= rcp_total;
        }
    };

    for (size_t pi = 0; pi < cpoints; ++pi) {
        int count = tmp.counts[pi];
        ret.counts[pi] = std::min(count, joints_per_vertex);
        ret.offsets[pi] = joints_per_vertex * pi;

        auto* src = &tmp.weights[tmp.offsets[pi]];
        auto* dst = &ret.weights[ret.offsets[pi]];
        if (count < joints_per_vertex) {
            copy(dst, src, size_t(count));
        }
        else {
            std::nth_element(src, src + joints_per_vertex, src + count,
                [](auto& a, auto& b) { return a.weight < b.weight; });
            normalize_weights(make_span(src, joints_per_vertex));
            copy(dst, src, size_t(joints_per_vertex));
        }
    }
    return ret;
}

JointMatrices Skin::getJointMatrices()
{
    JointMatrices ret;

    size_t cclusters = m_clusters.size();

    ret.bindpose.resize(cclusters);
    ret.global_transform.resize(cclusters);
    ret.joint_transform.resize(cclusters);
    for (size_t ci = 0; ci < cclusters; ++ci) {
        auto bindpose = m_clusters[ci]->getTransform();
        ret.bindpose[ci] = bindpose;
        if (auto trans = as<Model>(m_clusters[ci]->getChild())) {
            auto global_matrix = trans->getGlobalMatrix();
            ret.global_transform[ci] = global_matrix;
            ret.joint_transform[ci] = bindpose * global_matrix;
        }
        else {
            // should not be here
            sfbxPrint("sfbx::Deformer::skinMakeJointMatrices(): Cluster has non-Model child\n");
            ret.global_transform[ci] = ret.joint_transform[ci] = float4x4::identity();
        }
    }
    return ret;
}


Cluster* Skin::createCluster(Model* joint)
{
    auto r = createChild<Cluster>();
    r->addChild(joint);
    return r;
}

ObjectSubClass Cluster::getSubClass() const { return ObjectSubClass::Cluster; }

void Cluster::constructObject()
{
    super::constructObject();

    auto n = getNode();
    GetChildPropertyArray<int>(m_indices, n, sfbxS_Indexes);
    GetChildPropertyArray<float64>(m_weights, n, sfbxS_Weights);
    m_transform = GetChildPropertyValue<double4x4>(n, sfbxS_Transform);
    m_transform_link = GetChildPropertyValue<double4x4>(n, sfbxS_TransformLink);
}

void Cluster::constructNodes()
{
    super::constructNodes();

    auto n = getNode();
    n->createChild(sfbxS_Version, sfbxI_ClusterVersion);
    n->createChild(sfbxS_Mode, sfbxS_Total1);
    n->createChild(sfbxS_UserData, "", "");
    if (!m_indices.empty())
        n->createChild(sfbxS_Indexes, m_indices);
    if (!m_weights.empty())
        n->createChild(sfbxS_Weights, MakeAdaptor<float64>(m_weights));
    if (m_transform != float4x4::identity())
        n->createChild(sfbxS_Transform, (double4x4)m_transform);
    if (m_transform_link != float4x4::identity())
        n->createChild(sfbxS_TransformLink, (double4x4)m_transform_link);
}

void Cluster::addChild(Object* v)
{
    super::addChild(v);
    if (auto model = as<Model>(v))
        setName(v->getName());
}

span<int> Cluster::getIndices() const { return make_span(m_indices); }
span<float> Cluster::getWeights() const { return make_span(m_weights); }
float4x4 Cluster::getTransform() const { return m_transform; }
float4x4 Cluster::getTransformLink() const { return m_transform_link; }

void Cluster::setIndices(span<int> v) { m_indices = v; }
void Cluster::setWeights(span<float> v) { m_weights = v; }
void Cluster::setBindMatrix(float4x4 v)
{
    m_transform_link = v;
    m_transform = invert(v);
}


ObjectSubClass BlendShape::getSubClass() const { return ObjectSubClass::BlendShape; }

void BlendShape::constructObject()
{
    super::constructObject();
}

void BlendShape::constructNodes()
{
    super::constructNodes();

    auto n = getNode();
    n->createChild(sfbxS_Version, sfbxI_BlendShapeVersion);
}

void BlendShape::addChild(Object* v)
{
    super::addChild(v);
    if (auto ch = as<BlendShapeChannel>(v))
        m_channels.push_back(ch);
}

span<BlendShapeChannel*> BlendShape::getChannels() const
{
    return make_span(m_channels);
}

BlendShapeChannel* BlendShape::createChannel(string_view name)
{
    return createChild<BlendShapeChannel>(name);
}


ObjectSubClass BlendShapeChannel::getSubClass() const { return ObjectSubClass::BlendShapeChannel; }

void BlendShapeChannel::constructObject()
{
    super::constructObject();

    for (auto c : getChildren()) {
        if (auto shape = as<Shape>(c))
            m_shape_data.push_back({ shape, 100.0f });
    }
    if (auto n = getNode()->findChild(sfbxS_FullWeights)) {
        RawVector<float> weights;
        GetPropertyArray<float>(weights, n);
        if (weights.size() == m_shape_data.size()) {
            size_t n = weights.size();
            for (size_t i = 0; i < n; ++i)
                m_shape_data[i].weight = weights[i];
        }
    }
}

void BlendShapeChannel::constructNodes()
{
    super::constructNodes();

    auto n = getNode();
    n->createChild(sfbxS_Version, sfbxI_BlendShapeChannelVersion);
    n->createChild(sfbxS_DeformPercent, (float64)0);
    if (!m_shape_data.empty()) {
        auto full_weights = n->createChild(sfbxS_FullWeights);
        auto* dst = full_weights->createProperty()->allocateArray<float64>(m_shape_data.size()).data();
        for (auto& data : m_shape_data)
            *dst++ = data.weight;
    }
}

span<BlendShapeChannel::ShapeData> BlendShapeChannel::getShapeData() const
{
    return make_span(m_shape_data);
}

void BlendShapeChannel::addShape(Shape* shape, float weight)
{
    if (shape) {
        addChild(shape);
        m_shape_data.push_back({ shape, weight });
    }
}

Shape* BlendShapeChannel::createShape(string_view name, float weight)
{
    auto ret = createChild<Shape>(name);
    m_shape_data.push_back({ ret, weight });
    return ret;
}


ObjectClass Pose::getClass() const { return ObjectClass::Pose; }

ObjectSubClass BindPose::getSubClass() const { return ObjectSubClass::BindPose; }

void BindPose::constructObject()
{
    super::constructObject();

    for (auto n : getNode()->getChildren()) {
        if (n->getName() == sfbxS_PoseNode) {
            auto nid = GetChildPropertyValue<int64>(n, sfbxS_Node);
            auto mat = GetChildPropertyValue<double4x4>(n, sfbxS_Matrix);
            auto model = as<Model>(m_document->findObject(nid));
            if (model) {
                m_pose_data.push_back({ model, float4x4(mat) });
            }
            else {
                sfbxPrint("sfbx::Pose::constructObject(): non-Model joint object\n");
            }
        }
    }
}

void BindPose::constructNodes()
{
    super::constructNodes();

    auto n = getNode();
    n->createChild(sfbxS_Type, sfbxS_BindPose);
    n->createChild(sfbxS_Version, sfbxI_BindPoseVersion);
    n->createChild(sfbxS_NbPoseNodes, (int32)m_pose_data.size());
    for (auto& d : m_pose_data) {
        auto pn = n->createChild(sfbxS_PoseNode);
        pn->createChild(sfbxS_Node, (int64)d.object);
        pn->createChild(sfbxS_Matrix, (double4x4)d.matrix);
    }
}

span<BindPose::PoseData> BindPose::getPoseData() const { return make_span(m_pose_data); }
void BindPose::addPoseData(Model* joint, float4x4 bind_matrix) { m_pose_data.push_back({ joint, bind_matrix }); }

} // namespace sfbx

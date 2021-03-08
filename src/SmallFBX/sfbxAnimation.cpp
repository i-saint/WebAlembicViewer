#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxObject.h"
#include "sfbxDocument.h"

namespace sfbx {


ObjectClass AnimationStack::getClass() const { return ObjectClass::AnimationStack; }

void AnimationStack::addChild(Object* v)
{
    super::addChild(v);
    if (auto l = as<AnimationLayer>(v))
        m_anim_layers.push_back(l);
}

span<AnimationLayer*> AnimationStack::getAnimationLayers() const
{
    return m_anim_layers;
}

AnimationLayer* AnimationStack::createLayer(string_view name)
{
    return createChild<AnimationLayer>(name);
}


ObjectClass AnimationLayer::getClass() const { return ObjectClass::AnimationLayer; }

void AnimationLayer::constructObject()
{
    super::constructObject();
}

void AnimationLayer::constructNodes()
{
    super::constructNodes();
}

void AnimationLayer::addChild(Object* v)
{
    super::addChild(v);
    if (auto acn = as<AnimationCurveNode>(v))
        m_anim_nodes.push_back(acn);
}

span<AnimationCurveNode*> AnimationLayer::getAnimationCurveNodes() const
{
    return make_span(m_anim_nodes);
}

AnimationCurveNode* AnimationLayer::createCurveNode(AnimationKind kind, Object* target)
{
    auto ret = createChild<AnimationCurveNode>();
    ret->initialize(kind, target);
    return ret;
}


struct AnimationKindData
{
    std::string object_name;
    std::string link_name;
    std::vector<std::string> curve_names;
};
static const AnimationKindData g_akdata[] = {
    {}, // Unknown
    {sfbxS_T, sfbxS_LclTranslation, {"d|X", "d|Y", "d|Z"}}, // Position
    {sfbxS_R, sfbxS_LclRotation, {"d|X", "d|Y", "d|Z"}}, // Rotation
    {sfbxS_S, sfbxS_LclScale, {"d|X", "d|Y", "d|Z"}}, // Scale
    {sfbxS_DeformPercent, sfbxS_DeformPercent, {"d|" sfbxS_DeformPercent}}, // Weight
    {sfbxS_FocalLength, sfbxS_FocalLength, {"d|" sfbxS_FocalLength}}, // Weight
};


ObjectClass AnimationCurveNode::getClass() const { return ObjectClass::AnimationCurveNode; }

void AnimationCurveNode::constructObject()
{
    super::constructObject();

    auto name = getDisplayName();
    for (int i = 0; i < std::size(g_akdata); ++i) {
        if (name == g_akdata[i].object_name) {
            m_kind = (AnimationKind)i;
            break;
        }
    }
    if (m_kind == AnimationKind::Unknown) {
        sfbxPrint("sfbx::AnimationCurveNode: unrecognized animation target \"%s\"\n", std::string(name).c_str());
    }
}

void AnimationCurveNode::constructNodes()
{
    super::constructNodes();
}

void AnimationCurveNode::constructLinks()
{
    // ignore super::constructLinks()

    m_document->createLinkOO(this, getParent());

    auto& acd = g_akdata[(int)m_kind];
    if (m_curves.size() == acd.curve_names.size()) {
        m_document->createLinkOP(this, m_target, acd.link_name);
        for (size_t i = 0; i < m_curves.size(); ++i)
            m_document->createLinkOP(m_curves[i], this, acd.curve_names[i]);
    }
}

void AnimationCurveNode::addChild(Object* v)
{
    super::addChild(v);
    if (auto curve = as<AnimationCurve>(v))
        m_curves.push_back(curve);
}

AnimationKind AnimationCurveNode::getKind() const
{
    return m_kind;
}

float AnimationCurveNode::getStartTime() const
{
    return m_curves.empty() ? 0.0f : m_curves[0]->getStartTime();
}

float AnimationCurveNode::getEndTime() const
{
    return m_curves.empty() ? 0.0f : m_curves[0]->getEndTime();
}

float AnimationCurveNode::evaluate(float time) const
{
    if (m_curves.empty())
        return 0.0f;
    return m_curves[0]->evaluate(time);
}

float3 AnimationCurveNode::evaluate3(float time) const
{
    if (m_curves.size() != 3)
        return float3::zero();

    return float3{
        m_curves[0]->evaluate(time),
        m_curves[1]->evaluate(time),
        m_curves[2]->evaluate(time),
    };
}

void AnimationCurveNode::applyAnimation(float time) const
{
    if (m_curves.empty() || m_kind == AnimationKind::Unknown)
        return;

    switch (m_kind) {
    case AnimationKind::Position:
        if (auto* model = as<Model>(m_target))
            model->setPosition(evaluate3(time));
        break;
    case AnimationKind::Rotation:
        if (auto* model = as<Model>(m_target))
            model->setRotation(evaluate3(time));
        break;
    case AnimationKind::Scale:
        if (auto* model = as<Model>(m_target))
            model->setScale(evaluate3(time));
        break;
    case AnimationKind::DeformWeight:
        if (auto* bsc = as<BlendShapeChannel>(m_target))
            bsc->setWeight(evaluate(time));
        break;
    case AnimationKind::FocalLength:
        if (auto cam = as<Camera>(m_target)) {
            // todo
        }
        break;
    default:
        // should not be here
        sfbxPrint("sfbx::AnimationCurveNode: something wrong\n");
        break;
    }
}

void AnimationCurveNode::initialize(AnimationKind kind, Object* target)
{
    m_kind = kind;
    m_target = target;
    m_target->addChild(this);

    auto& acd = g_akdata[(int)m_kind];
    setName(acd.object_name);
    for (auto& cn : acd.curve_names)
        createChild<AnimationCurve>();
}

void AnimationCurveNode::addValue(float time, float value)
{
    if (m_curves.size() != 1) {
        sfbxPrint("afbx::AnimationCurveNode::addValue() curve count mismatch\n");
        return;
    }
    m_curves[0]->addValue(time, value);
}

void AnimationCurveNode::addValue(float time, float3 value)
{
    if (m_curves.size() != 3) {
        sfbxPrint("afbx::AnimationCurveNode::addValue() curve count mismatch\n");
        return;
    }
    m_curves[0]->addValue(time, value.x);
    m_curves[1]->addValue(time, value.y);
    m_curves[2]->addValue(time, value.z);
}


ObjectClass AnimationCurve::getClass() const { return ObjectClass::AnimationCurve; }

void AnimationCurve::constructObject()
{
    super::constructObject();

    for (auto n : getNode()->getChildren()) {
        auto name = n->getName();
        if (name == sfbxS_Default) {
            m_default = (float32)GetPropertyValue<float64>(n);
        }
        else if (name == sfbxS_KeyTime) {
            RawVector<int64> times_i64;
            GetPropertyValue<int64>(times_i64, n);
            transform(m_times, times_i64, [](int64 v) { return float((double)v / sfbxI_TicksPerSecond); });
        }
        else if (name == sfbxS_KeyValueFloat) {
            GetPropertyValue<float32>(m_values, n);
        }
    }
}

void AnimationCurve::constructNodes()
{
    super::constructNodes();

    auto n = getNode();
    n->createChild(sfbxS_Default, (float64)m_default);
    n->createChild(sfbxS_KeyVer, sfbxI_KeyVer);
    n->createChild(sfbxS_KeyTime, MakeAdaptor<float64>(m_times));
    n->createChild(sfbxS_KeyValueFloat, m_values); // float array

    int attr_flags[] = { 24836 };
    float attr_data[] = { 0, 0, 0, 0 };
    int attr_refcount[] = { (int)m_times.size() };
    n->createChild(sfbxS_KeyAttrFlags, make_span(attr_flags));
    n->createChild(sfbxS_KeyAttrDataFloat, make_span(attr_data));
    n->createChild(sfbxS_KeyAttrRefCount, make_span(attr_refcount));
}

void AnimationCurve::constructLinks()
{
    // do nothing
}

span<float> AnimationCurve::getTimes() const { return make_span(m_times); }
span<float> AnimationCurve::getValues() const { return make_span(m_values); }

float AnimationCurve::getStartTime() const { return m_times.empty() ? 0.0f : m_times.front(); }
float AnimationCurve::getEndTime() const { return m_times.empty() ? 0.0f : m_times.back(); }

float AnimationCurve::evaluate(float time) const
{
    if (m_times.empty())
        return m_default;
    else if (time <= m_times.front())
        return m_values.front();
    else if (time >= m_times.back())
        return m_values.back();
    else {
        // lerp
        auto it = std::lower_bound(m_times.begin(), m_times.end(), time);
        size_t i = std::distance(m_times.begin(), it);

        float t2 = m_times[i];
        float v2 = m_values[i];
        if (time == t2)
            return v2;

        float t1 = m_times[i - 1];
        float v1 = m_values[i - 1];
        float w = (time - t1) / (t2 - t1);
        return v1 + (v2 - v1) * w;
    }
}

void AnimationCurve::setTimes(span<float> v) { m_times = v; }
void AnimationCurve::setValues(span<float> v) { m_values = v; }

void AnimationCurve::addValue(float time, float value)
{
    m_times.push_back(time);
    m_values.push_back(value);
}

} // namespace sfbx

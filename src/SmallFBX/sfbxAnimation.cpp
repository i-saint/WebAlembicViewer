#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxObject.h"
#include "sfbxDocument.h"

namespace sfbx {


ObjectClass AnimationStack::getClass() const { return ObjectClass::AnimationStack; }


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


ObjectClass AnimationCurveNode::getClass() const { return ObjectClass::AnimationCurveNode; }

void AnimationCurveNode::constructObject()
{
    super::constructObject();

    auto name = getDisplayName();
    if (name == sfbxS_T)
        m_target = AnimationTarget::Position;
    else if (name == sfbxS_R)
        m_target = AnimationTarget::Rotation;
    else if (name == sfbxS_S)
        m_target = AnimationTarget::Scale;
    else if (name == sfbxS_DeformPercent)
        m_target = AnimationTarget::DeformWeight;
    else if (name == sfbxS_FocalLength)
        m_target = AnimationTarget::FocalLength;
    else
        sfbxPrint("sfbx::AnimationCurveNode: unrecognized animation target \"%s\"\n", std::string(name).c_str());
}

void AnimationCurveNode::constructNodes()
{
    super::constructNodes();

    if (m_parent_model && m_target != AnimationTarget::Unknown) {
        int64 pid = m_parent_model->getID();
        switch (m_target) {
        case AnimationTarget::Position:
            setName(sfbxS_T);
            addLinkOP(pid, sfbxS_LclTranslation);
            break;
        case AnimationTarget::Rotation:
            setName(sfbxS_R);
            addLinkOP(pid, sfbxS_LclRotation);
            break;
        case AnimationTarget::Scale:
            setName(sfbxS_S);
            addLinkOP(pid, sfbxS_LclScale);
            break;
        case AnimationTarget::DeformWeight:
            setName(sfbxS_DeformPercent);
            addLinkOP(pid, sfbxS_DeformPercent);
            break;
        case AnimationTarget::FocalLength:
            setName(sfbxS_FocalLength);
            addLinkOP(pid, sfbxS_FocalLength);
            break;
        default: break;
        }
    }
}

void AnimationCurveNode::addChild(Object* v)
{
    super::addChild(v);
    if (auto curve = as<AnimationCurve>(v))
        m_curves.push_back(curve);
}

void AnimationCurveNode::addParent(Object* v)
{
    super::addParent(v);
    if (auto model = as<Model>(v))
        m_parent_model = model;
    if (auto bschannel = as<BlendShapeChannel>(v))
        m_parent_bschannel = bschannel;
}

AnimationTarget AnimationCurveNode::getTarget() const
{
    return m_target;
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

void AnimationCurveNode::apply(float time) const
{
    if (m_curves.empty() || m_target == AnimationTarget::Unknown)
        return;

    switch (m_target) {
    case AnimationTarget::Position:
        if (m_parent_model)
            m_parent_model->setPosition(evaluate3(time));
        break;
    case AnimationTarget::Rotation:
        if (m_parent_model)
            m_parent_model->setRotation(evaluate3(time));
        break;
    case AnimationTarget::Scale:
        if (m_parent_model)
            m_parent_model->setScale(evaluate3(time));
        break;
    case AnimationTarget::DeformWeight:
        if (m_parent_bschannel)
            m_parent_bschannel->setWeight(evaluate(time));
        break;
    case AnimationTarget::FocalLength:
        // todo
        break;
    default:
        // should not be here
        sfbxPrint("sfbx::AnimationCurveNode: something wrong\n");
        break;
    }
}

void AnimationCurveNode::setTarget(AnimationTarget v)
{
    m_target = v;
}

void AnimationCurveNode::addValue(float time, float value)
{
    if (m_curves.empty()) {
        createChild<AnimationCurve>();
    }
    else if (m_curves.size() != 1) {
        sfbxPrint("afbx::AnimationCurveNode::addValue() curve count mismatch\n");
        return;
    }
    m_curves[0]->addValue(time, value);
}

void AnimationCurveNode::addValue(float time, float3 value)
{
    if (m_curves.empty()) {
        createChild<AnimationCurve>();
        createChild<AnimationCurve>();
        createChild<AnimationCurve>();
    }
    else if (m_curves.size() != 3) {
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
    // todo
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

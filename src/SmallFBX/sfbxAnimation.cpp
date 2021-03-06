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
    for (auto* n : getChildren()) {
        if (auto* cnode = as<AnimationCurveNode>(n)) {
            auto name = cnode->getDisplayName();
            if (name == sfbxS_T)
                m_position = cnode;
            else if (name == sfbxS_R)
                m_rotation = cnode;
            else if (name == sfbxS_S )
                m_scale = cnode;
            else if (name == sfbxS_FocalLength)
                m_focal_length = cnode;
        }
    }
}

void AnimationLayer::constructNodes()
{
    super::constructNodes();
    // todo
}

AnimationCurveNode* AnimationLayer::getPosition() const     { return m_position; }
AnimationCurveNode* AnimationLayer::getRotation() const     { return m_rotation; }
AnimationCurveNode* AnimationLayer::getScale() const        { return m_scale; }
AnimationCurveNode* AnimationLayer::getFocalLength() const  { return m_focal_length; }


ObjectClass AnimationCurveNode::getClass() const { return ObjectClass::AnimationCurveNode; }

void AnimationCurveNode::constructObject()
{
    super::constructObject();
}

void AnimationCurveNode::constructNodes()
{
    super::constructNodes();
    // todo
}

void AnimationCurveNode::addChild(Object* v)
{
    super::addChild(v);
    if (auto curve = as<AnimationCurve>(v))
        m_curves.push_back(curve);
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

    auto n = getNode();
    m_default = (float32)GetChildPropertyValue<float64>(n, sfbxS_Default);
    transform(m_times, GetChildPropertyArray<int64>(n, sfbxS_KeyTime),
        [](int64 v) { return float((double)v / sfbxI_TicksPerSecond); });
    m_values = GetChildPropertyArray<float32>(n, sfbxS_KeyValueFloat);
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

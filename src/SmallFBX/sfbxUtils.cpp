#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxObject.h"
#include "sfbxUtil.h"

namespace sfbx {

RawVector<int> Triangulate(span<int> counts, span<int> indices)
{
    size_t num_triangles = 0;
    for (int c : counts) {
        if (c >= 3)
            num_triangles += c - 2;
    }

    RawVector<int> ret(num_triangles * 3);
    int* dst = ret.data();
    for (int c : counts) {
        if (c >= 3) {
            for (int fi = 0; fi < c - 2; ++fi) {
                *dst++ = indices[0];
                *dst++ = indices[1 + fi];
                *dst++ = indices[2 + fi];
            }
        }
    }
    return ret;
}

// Mul: e.g. [](float4x4, float3) -> float3
template<class Vec, class Mul>
static bool DeformImpl(span<Vec> dst, const JointWeights& jw, const JointMatrices& jm, span<Vec> src, const Mul& mul)
{
    if (jw.counts.size() != src.size() || jw.counts.size() != dst.size()) {
        printf("Skin::deformImpl(): vertex count mismatch\n");
        return false;
    }

    const JointWeight* weights = jw.weights.data();
    const float4x4* matrices = jm.joint_transform.data();
    size_t nvertices = src.size();
    for (size_t vi = 0; vi < nvertices; ++vi) {
        Vec p = src[vi];
        Vec r{};
        int cjoints = jw.counts[vi];
        for (int bi = 0; bi < cjoints; ++bi) {
            JointWeight w = weights[bi];
            r += mul(matrices[w.index], p) * w.weight;
        }
        dst[vi] = r;
        weights += cjoints;
    }
    return true;
}

bool DeformPoints(span<float3> dst, const JointWeights& jw, const JointMatrices& jm, span<float3> src)
{
    return DeformImpl(dst, jw, jm, src,
        [](float4x4 m, float3 p) { return mul_p(m, p); });
}

bool DeformVectors(span<float3> dst, const JointWeights& jw, const JointMatrices& jm, span<float3> src)
{
    return DeformImpl(dst, jw, jm, src,
        [](float4x4 m, float3 p) { return mul_v(m, p); });
}

} // namespace sfbx

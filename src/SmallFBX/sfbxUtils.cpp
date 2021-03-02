#include "pch.h"
#include "sfbxInternal.h"
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

} // namespace sfbx

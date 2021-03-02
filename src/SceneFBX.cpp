#include "pch.h"
#include "SceneGraph.h"
#include "SmallFBX.h"

namespace wabc {

using sfbx::as;

class SceneFBX : public IScene
{
public:
    struct ImportContext
    {
        sfbx::Object* obj;
        double time = 0.0;
        float4x4 local_matrix = float4x4::identity();
        float4x4 global_matrix = float4x4::identity();
    };

    void release() override;

    bool load(const char* path) override;
    void unload() override;

    std::tuple<double, double> getTimeRange() const override;
    void seek(double time) override;

    double getTime() const override { return m_time; }
    IMesh* getMesh() override { return m_mono_mesh.get(); }
    IPoints* getPoints() override { return nullptr; }
    span<ICamera*> getCameras() override { return make_span(m_cameras); }

private:
    // ctx is not a reference. that is intended.
    void scanObjects(ImportContext ctx);

    sfbx::DocumentPtr m_document;
    std::tuple<double, double> m_time_range;

    double m_time = -1.0;
    MeshPtr m_mono_mesh;

    std::map<std::string, CameraPtr> m_camera_table;
    std::vector<ICamera*> m_cameras;
};

template<class Cont> inline auto expand(Cont& v, size_t n)
{
    size_t pos = v.size();
    v.resize(pos + n);
    return v.data() + pos;
}


void SceneFBX::release()
{
    delete this;
}

// todo
void SceneFBX::scanObjects(ImportContext ctx)
{
    auto obj = ctx.obj;
    if (auto model = as<sfbx::Model>(obj)) {
        auto m = model->getLocalMatrix();
        ctx.local_matrix = (float4x4&)m;
        ctx.global_matrix = ctx.local_matrix * ctx.global_matrix;

        if (auto cam = as<sfbx::Camera>(model)) {
            // todo
        }
    }
    else if (auto mesh = as<sfbx::Mesh>(obj)) {
        auto counts = mesh->getCounts();
        auto indices = mesh->getIndices();
        auto points = mesh->getPoints();

        // make points in global space
        int num_faces = (int)counts.size();
        int num_indices = (int)indices.size();
        int num_points = (int)points.size();
        int index_offset = (int)m_mono_mesh->m_points.size();
        float3* dst_points = expand(m_mono_mesh->m_points, num_points);
        for (int i = 0; i < num_points; ++i)
            dst_points[i] = mul_p(ctx.global_matrix, (float3&)points[i]);

        // count primitives and allocate space
        int num_lines = 0;
        int num_triangles = 0;
        for (int c : counts) {
            if (c == 2) {
                num_lines += 1;
            }
            else if (c >= 3) {
                num_triangles += c - 2;
                num_lines += c;
            }
        }


        const float3* src_points = dst_points;
        const int* src_indices = indices.data();
        int* dst_counts = expand(m_mono_mesh->m_counts, num_faces);
        int* dst_findices = expand(m_mono_mesh->m_face_indices, num_indices);
        int* dst_windices = expand(m_mono_mesh->m_wireframe_indices, num_lines * 2);
        float3* dst_points_ex = expand(m_mono_mesh->m_points_ex, num_triangles * 3);

        // setup indices & vertices

        for (int i = 0; i < num_faces; ++i)
            dst_counts[i] = counts[i];

        for (int i = 0; i < num_indices; ++i)
            dst_findices[i] = src_indices[i] + index_offset;

        for (int c : counts) {
            if (c == 2) {
                // add wire frame indices
                *dst_windices++ = src_indices[0] + index_offset;
                *dst_windices++ = src_indices[1] + index_offset;
            }
            else if (c > 2) {
                // add wire frame indices
                for (int fi = 0; fi < c; ++fi) {
                    *dst_windices++ = src_indices[fi] + index_offset;
                    *dst_windices++ = (fi == c - 1 ? src_indices[0] : src_indices[fi + 1]) + index_offset;
                }

                // add triangle vertices
                // todo: handle flip faces option
                for (int fi = 0; fi < c - 2; ++fi) {
                    int i0 = src_indices[0];
                    int i1 = src_indices[1 + fi];
                    int i2 = src_indices[2 + fi];
                    *dst_points_ex++ = src_points[i0];
                    *dst_points_ex++ = src_points[i1];
                    *dst_points_ex++ = src_points[i2];
                }
            }
            src_indices += c;
        }
    }
    else if (auto mesh = as<sfbx::Skin>(obj)) {
        // todo
    }

    for (auto child : obj->getChildren()) {
        ctx.obj = child;
        scanObjects(ctx);
    }
}

bool SceneFBX::load(const char* path)
{
    unload();

    m_document = sfbx::MakeDocument();
    if (!m_document->read(path)) {
        unload();
        return false;
    }

    m_mono_mesh = std::make_shared<Mesh>();

    ImportContext ctx;
    for (auto obj : m_document->getRootObjects()) {
        ctx.obj = obj;
        scanObjects(ctx);
    }
    m_mono_mesh->upload();

    return true;
}

void SceneFBX::unload()
{
    m_document = nullptr;
    m_time_range = {};

    m_time = -1.0;
    m_mono_mesh = {};

    m_cameras = {};
    m_camera_table = {};
}

std::tuple<double, double> SceneFBX::getTimeRange() const
{
    return m_time_range;
}

void SceneFBX::seek(double time)
{
    if (!m_document || time == m_time)
        return;

    // todo: handle animation

    //m_time = time;
    //m_mono_mesh->clear();

    //ImportContext ctx;
    //ctx.time = time;
    //for (auto obj : m_document->getRootObjects()) {
    //    ctx.obj = obj;
    //    scanObjects(ctx);
    //}

    //m_mono_mesh->upload();
}

IScene* CreateSceneFBX_()
{
    return new SceneFBX();
}

} // namespace wabc

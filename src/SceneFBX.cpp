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

    struct MeshData
    {
        sfbx::GeomMesh* mesh_fbx{};
        RawVector<int> indices_tri;
        size_t points_offset{};
        size_t pointsex_offset{};
    };
    using MeshDataPtr = std::shared_ptr<MeshData>;

    struct SkinData
    {
        sfbx::Skin* skin_fbx{};
        sfbx::JointWeights weights;
        sfbx::JointMatrices matrices;
        RawVector<float3> points_deformed;
        MeshDataPtr mesh;
    };
    using SkinDataPtr = std::shared_ptr<SkinData>;

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
    void applyDeform();

    sfbx::DocumentPtr m_document;
    std::tuple<double, double> m_time_range;

    double m_time = -1.0;
    MeshPtr m_mono_mesh;
    std::vector<MeshDataPtr> m_mesh_data;
    std::vector<SkinDataPtr> m_skin_data;

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
    else if (auto mesh = as<sfbx::GeomMesh>(obj)) {
        auto tmp = std::make_shared<MeshData>();
        tmp->mesh_fbx = mesh;
        tmp->points_offset = m_mono_mesh->m_points.size();
        tmp->pointsex_offset = m_mono_mesh->m_points_ex.size();
        m_mesh_data.push_back(tmp);

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
        int* dst_indicex_ex = expand(tmp->indices_tri, num_triangles * 3);

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
                    *dst_indicex_ex++ = i0;
                    *dst_indicex_ex++ = i1;
                    *dst_indicex_ex++ = i2;
                    *dst_points_ex++ = src_points[i0];
                    *dst_points_ex++ = src_points[i1];
                    *dst_points_ex++ = src_points[i2];
                }
            }
            src_indices += c;
        }
    }
    else if (auto skin = as<sfbx::Skin>(obj)) {
        auto tmp = std::make_shared<SkinData>();
        tmp->skin_fbx = skin;
        auto mesh_fbx = skin->getMesh();
        for (auto& p : m_mesh_data) {
            if (p->mesh_fbx == mesh_fbx) {
                tmp->mesh = p;
                break;
            }
        }
        tmp->weights = skin->getJointWeights();
        tmp->points_deformed.resize(tmp->weights.counts.size());

        m_skin_data.push_back(tmp);
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

void SceneFBX::applyDeform()
{
    bool needs_upload = false;

    // skin
    for (auto& skin : m_skin_data) {
        if (!skin->mesh || skin->mesh->indices_tri.empty())
            continue;

        skin->matrices = skin->skin_fbx->getJointMatrices();
        auto mesh = skin->mesh;
        {
            auto src = mesh->mesh_fbx->getPoints();
            auto dst = make_span((sfbx::float3*)skin->points_deformed.data(), skin->points_deformed.size());
            sfbx::DeformPoints(dst, skin->weights, skin->matrices, src);
        }
        {
            auto src = make_span(skin->points_deformed.data(), skin->points_deformed.size());
            auto dst = make_span(m_mono_mesh->m_points.data() + mesh->points_offset, m_mono_mesh->m_points.size());
            auto dst_ex = make_span(m_mono_mesh->m_points_ex.data() + mesh->pointsex_offset, mesh->indices_tri.size());
            sfbx::copy(dst, src);
            sfbx::copy_indexed(dst_ex, src, mesh->indices_tri);
        }
        needs_upload = true;
    }

    // todo: blend shape

    if (needs_upload)
        m_mono_mesh->upload();
}

void SceneFBX::seek(double time)
{
    if (!m_document || time == m_time)
        return;

    applyDeform();
}

IScene* CreateSceneFBX_()
{
    return new SceneFBX();
}

} // namespace wabc

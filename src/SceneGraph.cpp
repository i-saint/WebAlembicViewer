#include "pch.h"
#include "WebAlembicViewer.h"

using namespace Alembic;

namespace wabc {

template<class T> inline std::span<T> MakeSpan(const T& v) { return { (T*)&v, 1 }; }
template<class T> inline std::span<T> MakeSpan(const std::vector<T>& v) { return { (T*)v.data(), v.size() }; }
template<class T> inline std::span<T> MakeSpan(const T* v, size_t n) { return { (T*)v, n }; }

class Camera : public ICamera
{
public:
    float3 getPosition() const { return m_position; }
    float3 getDirection() const { return m_direction; }
    float3 getUp() const { return m_up; }
    float getFOV() const { return m_fov; }

private:
    float3 m_position{};
    float3 m_direction{ 0.0f, 0.0f, 1.0f };
    float3 m_up{ 0.0f, 1.0f, 0.0f };
    float m_fov = 30.0f;
};

class Mesh : public IMesh
{
public:
    float4x4 getTransform() const override { return m_transform; }
    std::span<float3> getPoints() const override { return MakeSpan(m_points); }
    std::span<int> getIndices() const override { return MakeSpan(m_indices); }

public:
    float4x4 m_transform = float4x4::identity();
    std::vector<float3> m_points;
    std::vector<int> m_indices;
};


class Scene : public IScene
{
public:
    struct AbcContext
    {
        float4x4 local_transform;
        float4x4 global_transform;
        Abc::IObject abcobj;
    };

    bool load(const char* path) override;
    void close() override;

    std::tuple<double, double> getTimeRange() const override;
    void seek(double time) override;


    void seekImpl(Abc::IObject obj, double time);

private:
    std::shared_ptr<std::fstream> m_stream;
    Abc::IArchive m_archive;
};


bool Scene::load(const char* path)
{
    close();

    try
    {
        // Abc::IArchive doesn't accept wide string path. so create file stream with wide string path and pass it.
        // (VisualC++'s std::ifstream accepts wide string)
        m_stream.reset(new std::fstream());
        m_stream->open(path, std::ios::in | std::ios::binary);
        if (!m_stream->is_open()) {
            close();
            return false;
        }

        std::vector< std::istream*> streams{ m_stream.get() };
        Alembic::AbcCoreOgawa::ReadArchive archive_reader(streams);
        m_archive = Abc::IArchive(archive_reader(path), Abc::kWrapExisting, Abc::ErrorHandler::kThrowPolicy);
        return true;
    }
    catch (Alembic::Util::Exception e)
    {
        close();

#ifdef wabcEnableHDF5
        try
        {
            m_archive = Abc::IArchive(AbcCoreHDF5::ReadArchive(), path);
        }
        catch (Alembic::Util::Exception e2)
        {
            close();
            //sgDbgPrint(
            //    "failed to open %s\n"
            //    "it may not an alembic file"
            //    , path);
            return false;
        }
#else
        //sgDbgPrint(
        //    "failed to open %s\n"
        //    "it may not an alembic file or not in Ogawa format (HDF5 is not supported)"
        //    , path);
        return false;
#endif
    }
    return false;
}

void Scene::close()
{
    m_archive = {};
    m_stream = {};
}

std::tuple<double, double> Scene::getTimeRange() const
{
    return { 0.0, 0.0 };
}

void Scene::seek(double time)
{
    if (!m_archive)
        return;

    seekImpl(m_archive.getTop(), time);
}

void Scene::seekImpl(Abc::IObject obj, double time)
{
    size_t n = obj.getNumChildren();
    for (size_t ci = 0; ci < n; ++ci) {
        auto child = obj.getChild(ci);
        const auto& metadata = child.getMetaData();

        if (AbcGeom::IXformSchema::matches(metadata)) {
            auto schema = AbcGeom::IXform(child).getSchema();
            AbcGeom::XformSample sample;
            schema.get(sample, time);
            auto matd = sample.getMatrix();

            float4x4 local_matrix;
            local_matrix.assign((double4x4&)matd);
        }
        else if (AbcGeom::IPolyMeshSchema::matches(metadata)) {
            auto schema = AbcGeom::IPolyMesh(child).getSchema();
            AbcGeom::IPolyMeshSchema::Sample sample;
            schema.get(sample, time);
        }
        else if (AbcGeom::IPointsSchema::matches(metadata)) {

        }
        else if (AbcMaterial::IMaterialSchema::matches(metadata)) {

        }
        else {
        }

        seekImpl(child, time);
    }
}

} // namespace wabc

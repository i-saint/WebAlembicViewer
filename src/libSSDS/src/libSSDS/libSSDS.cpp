#include "pch.h"
#include "ssdsMath.h"
#include "ssdsTypes.h"
#include <Eigen/Eigenvalues>

namespace ssds {

static float4x4 calcRegistrationT(span<float3> s, span<float3> d)
{
    size_t num_points = s.size();
    double3 cs{}, cd{}; // double for precision
    for (size_t i = 0; i < num_points; ++i) {
        cs += s[i];
        cd += d[i];
    }
    return translate(float3((cd - cs) / double(num_points)));
}

static float4x4 calcRegistrationRT(span<float3> s, span<float3> d)
{
    size_t num_points = s.size();
    double3 cs{}, cd{}; // double for precision
    for (size_t i = 0; i < num_points; ++i) {
        cs += s[i];
        cd += d[i];
    }
    cs /= double(num_points);
    cd /= double(num_points);
    if (num_points < 3)
        return translate(cd - cs);

    Eigen::Matrix<double, 4, 4> moment;
    {
        auto sit = s.data();
        auto dit = d.data();
        double sxx = 0, sxy = 0, sxz = 0, syx = 0, syy = 0, syz = 0, szx = 0, szy = 0, szz = 0;
        for (int i = 0; i < num_points; ++i, ++sit, ++dit) {
            sxx += (sit->x - cs.x) * (dit->x - cd.x);
            sxy += (sit->x - cs.x) * (dit->y - cd.y);
            sxz += (sit->x - cs.x) * (dit->z - cd.z);
            syx += (sit->y - cs.y) * (dit->x - cd.x);
            syy += (sit->y - cs.y) * (dit->y - cd.y);
            syz += (sit->y - cs.y) * (dit->z - cd.z);
            szx += (sit->z - cs.z) * (dit->x - cd.x);
            szy += (sit->z - cs.z) * (dit->y - cd.y);
            szz += (sit->z - cs.z) * (dit->z - cd.z);
        }
        moment(0, 0) = sxx + syy + szz;
        moment(0, 1) = syz - szy;        moment(1, 0) = moment(0, 1);
        moment(0, 2) = szx - sxz;        moment(2, 0) = moment(0, 2);
        moment(0, 3) = sxy - syx;        moment(3, 0) = moment(0, 3);
        moment(1, 1) = sxx - syy - szz;
        moment(1, 2) = sxy + syx;        moment(2, 1) = moment(1, 2);
        moment(1, 3) = szx + sxz;        moment(3, 1) = moment(1, 3);
        moment(2, 2) = -sxx + syy - szz;
        moment(2, 3) = syz + szy;        moment(3, 2) = moment(2, 3);
        moment(3, 3) = -sxx - syy + szz;
    }

    float4x4 transform;
    if (moment.norm() > 0) {
        Eigen::EigenSolver<Eigen::Matrix<double, 4, 4>> es(moment);
        int maxi = 0;
        for (int i = 1; i < 4; ++i) {
            if (es.eigenvalues()(maxi).real() < es.eigenvalues()(i).real()) {
                maxi = i;
            }
        }

        quatf rot = {
            (float)es.eigenvectors()(0, maxi).real(),
            (float)es.eigenvectors()(1, maxi).real(),
            (float)es.eigenvectors()(2, maxi).real(),
            (float)es.eigenvectors()(3, maxi).real(),
        };
        transform = to_mat4x4(rot);
    }
    float3 cs0 = mul_p(transform, float3(cs));
    (float3&)transform[3] = cd - cs0;
    return transform;
}

#if 0
float4x4 calcRegistrationSRT(span<float3> s, span<float3> d)
{
    size_t num_points = s.size();
    const int num_iterations = 50;
    const double threshold = 1.0e-8;
    const double scaleLowerBound = 1.0e-3;
    float4x4 transform = calcRegistrationRT(s, d);

    float3 scale = float3::one();
    float3 denom = float3::zero();
    {
        auto sit = s.data();
        for (int i = 0; i < num_points; ++i, ++sit) {
            denom[0] += (*sit)[0] * (*sit)[0];
            denom[1] += (*sit)[1] * (*sit)[1];
            denom[2] += (*sit)[2] * (*sit)[2];
        }
    }

    std::vector<float3> sstm(num_points);
    {
        auto sit = s.data();
        for (int i = 0; i < num_points; ++i, ++sit) {
            sstm[i][0] = (*sit)[0];
            sstm[i][1] = (*sit)[1];
            sstm[i][2] = (*sit)[2];
        }
    }

    std::vector<float3> dstm(num_points);
    for (size_t l = 0; l < num_iterations; ++l) {
        float4x4 im = invert(transform);
        auto dit = d.data();
        for (size_t i = 0; i < num_points; ++i, ++dit)
            dstm[i] = mul_p(im, *dit);

        for (size_t d = 0; d < 3; ++d) {
            double ds = 0.0;
            auto sit = s.data();
            for (size_t i = 0; i < num_points; ++i, ++sit) {
                ds += (*sit)[d] * dstm[i][d];
            }
            scale[d] = denom[d] < threshold ? 1.0 : ds / denom[d];
            sit = s.data();
            for (size_t i = 0; i < num_points; ++i, ++sit) {
                sstm[i][d] = scale[d] * (*sit)[d];
            }
        }
        transform = calcRegistrationRT(num_points, sstm.begin(), pd);
    }
    double sv[3] = { scale[0], scale[1], scale[2] };
    transform.setScale(sv, MSpace::kTransform);
    return transform;
}

using RegistrationFunc = float4x4(*)(span<float3> ps, span<float3> pd);

static RegistrationFunc registrationFuncs[3] = {
    calcRegistrationT,
    calcRegistrationRT,
    calcRegistrationSRT
};

void computeSamplePoints(std::vector<float3>& sample, int sid, int joint, const Output& output, const Input& input)
{
    const int num_vertices = input.num_vertices;
    const int& num_indices = output.num_indices;

    for (int v = 0; v < num_vertices; ++v) {
        sample[v] = input.sample[sid * num_vertices + v];
        const float3& s = input.rest_shape[v];
        for (int i = 0; i < num_indices; ++i) {
            const int jnt = output.skin_indices[v * num_indices + i];
            if (jnt >= 0 && jnt != joint) {
                const double w = output.skin_weights[v * num_indices + i];
                const float4x4& at = output.skinMatrix[sid * input.num_joints + jnt];
                sample[v] -= w * (s * at.asMatrix());
            }
        }
    }
}

void subtractCentroid(std::vector<float3>& model, std::vector<float3>& sample, float3& corModel, float3& corSample, const Eigen::VectorXd& weight, const Output& output, const Input& input)
{
    const int num_vertices = input.num_vertices;

    double wsqsum = 0;
    corModel = {};
    corSample = {};
    for (int v = 0; v < num_vertices; ++v) {
        const double w = weight[v];
        corModel += w * w * input.restShape[v];
        corSample += w * sample[v];
        wsqsum += w * w;
    }
    corModel = corModel / wsqsum;
    corSample = corSample / wsqsum;
    for (int v = 0; v < num_vertices; ++v) {
        model[v] = weight[v] * (input.restShape[v] - corModel);
        sample[v] -= weight[v] * corSample;
    }
}


class JointTransformUpdator
{
private:
    Output* output;
    const Input* input;
    const Eigen::VectorXd* weight;
    int transformType;
    int joint;

public:
    JointTransformUpdator(Output* output_, const Input* input_,
        const Eigen::VectorXd* weight_, int joint_, int transformType_)
        : output(output_), input(input_),
        weight(weight_), joint(joint_), transformType(transformType_)
    {
    }

    void operator ()(const tbb::blocked_range<int>& range) const
    {
        std::vector<float3> model(input->num_vertices);
        std::vector<float3> sample(input->num_vertices);
        for (int s = range.begin(); s != range.end(); ++s) {
            if (s == 0) {
                output->skinMatrix[joint] = float4x4::identity;
                continue;
            }
            computeSamplePoints(sample, s, joint, *output, *input);
            float3 corModel(0, 0, 0), corSample(0, 0, 0);
            subtractCentroid(model, sample, corModel, corSample, *weight, *output, *input);
            float4x4 transform = registrationFuncs[transformType](model.size(), model.begin(), sample.begin());
            float3 d = corSample - corModel * transform.asMatrix();
            transform.setTranslation(d + transform.getTranslation(MSpace::kTransform), MSpace::kTransform);
            output->skinMatrix[s * input->num_joints + joint] = transform;
        }
    }
};

void updateJointTransformProc(Output& output, int transformType, const Input& input, int selection = -1)
{
    const int num_vertices = input.sample.front().points.size();
    const int num_samples  = input.sample.size();
    const int num_joints    = output.num_joints;
    const int num_indices  = output.num_indices;

    Eigen::VectorXd weight = Eigen::VectorXd::Zero(num_vertices);
    int begin = selection < 0 ? 0 : selection;
    int end = selection < 0 ? num_joints : selection + 1;
    for (int joint = begin; joint < end; ++joint) {
        for (int v = 0; v < num_vertices; ++v) {
            weight[v] = 0.0;
            for (int i = 0; i < num_indices; ++i) {
                int jnt = output.skin_indices[v * num_indices + i];
                if (jnt == joint) {
                    weight[v] = output.skin_weights[v * num_indices + i];
                    break;
                }
            }
        }
        double wsqsum = weight.dot(weight);
        if (wsqsum > 1.0e-8) {
            tbb::blocked_range<int> blockedRange(0, num_samples);
            JointTransformUpdator transformUpdator(&output, &input, &weight, joint, transformType);
            tbb::parallel_for(blockedRange, transformUpdator);
        }
        else {
            for (int s = 0; s < num_samples; ++s) {
                output.skinMatrix[s * input.num_joints + joint] = float4x4::identity;
            }
            char buf[256];
            printf(buf, "Fixed joint #%d", joint);
        }
    }
}

static PyObject* updateJointTransform(PyObject *self, PyObject *args)
{
    PyObject* pin            = PyTuple_GET_ITEM(args, 0);
    PyObject* pout           = PyTuple_GET_ITEM(args, 1);
    const int transformType = PyInt_AsLong(PyTuple_GET_ITEM(args, 2));
    Input* const input   = reinterpret_cast<Input*>(PyCapsule_GetPointer(pin, "SSDSInput"));
    Output* const output = reinterpret_cast<Output*>(PyCapsule_GetPointer(pout, "SSDSOutput"));
    updateJointTransformProc(*output, transformType, *input);
    return Py_None;
}


// see https://sites.google.com/view/fumiyanarita/project/la_ssdr_mdmc
static void detectNeighborClusters(const Input& input, Output& output)
{
    MGlobal::displayInfo("Detecting neighbor clusters");
    const int& num_indices = output.num_indices;

    std::vector<std::set<int>> clusters(output.num_joints);
    for (int v = 0; v < input.num_vertices; ++v) {
        const int primjid = output.skinIndex[v * num_indices];
        if (input.numRings == 0) {
            for (int j = 0; j < output.num_joints; ++j) {
                for (int v = 0; v < output.num_joints; ++v) {
                    clusters[j].insert(v);
                }
            }
        }
        else {
            clusters[primjid].insert(0);
            clusters[primjid].insert(primjid);
            // one-ring neighbors
            for (auto nvit = input.neighbor[v].begin(); nvit != input.neighbor[v].end(); ++nvit) {
                const int nvid = nvit->first;
                clusters[primjid].insert(output.skinIndex[nvid * num_indices]);
                if (input.numRings == 1) {
                    continue;
                }
                // two-ring neighbors
                for (auto nnvit = input.neighbor[nvid].begin(); nnvit != input.neighbor[nvid].end(); ++nnvit) {
                    const int nnvid = nnvit->first;
                    clusters[primjid].insert(output.skinIndex[nnvid * num_indices]);
                }
            }
        }
    }
    for (int v = 0; v < input.num_vertices; ++v) {
        const int primjid = output.skinIndex[v * num_indices];
        output.vertCluster[v] = std::vector<int>(clusters[primjid].begin(), clusters[primjid].end());
    }
}

int bindVertexToJoint(Output& output, const Input& input)
{
    const int num_vertices = input.num_vertices;
    const int num_samples  = input.num_samples;
    const int num_indices  = output.num_indices;

    std::vector<int> numJointVertices(output.num_joints, 0);
    for (int v = 0; v < num_vertices; ++v) {
        int bestJoint = 0;
        double minErr = std::numeric_limits<double>::max();
        const float3& restShapePos = input.restShape[v];
        for (int j = 0; j < output.num_joints; ++j) {
            double errsq = 0;
            for (int s = 0; s < num_samples; ++s) {
                float4x4 am = output.skinMatrix[s * input.num_joints + j].asMatrix();
                float3 diff = input.sample[s * num_vertices + v] - restShapePos * am;
                errsq += diff * diff;
            }
            errsq *= (input.restShape[v] - output.restJointPos[j]).length();
            if (errsq < minErr) {
                bestJoint = j;
                minErr = errsq;
            }
        }
        ++numJointVertices[bestJoint];
        output.skinIndex[v * num_indices + 0] = bestJoint;
    }

    std::vector<int>::iterator smallestBoneSize = std::min_element(numJointVertices.begin(), numJointVertices.end());
    while (*smallestBoneSize <= 0) {
        const int smallestBone = static_cast<int>(smallestBoneSize - numJointVertices.begin());
        numJointVertices.erase(numJointVertices.begin() + smallestBone);
        output.restJointPos.erase(output.restJointPos.begin() + smallestBone);
        for (int s = 0; s < input.num_samples; ++s) {
            for (int j = output.num_joints - 2; j >= smallestBone; --j) {
                output.skinMatrix[s * input.num_joints + j] = output.skinMatrix[s * input.num_joints + j + 1];
            }
        }
        for (int v = 0; v < num_vertices; ++v) {
            if (output.skinIndex[v * num_indices + 0] >= smallestBone) {
                --output.skinIndex[v * num_indices + 0];
            }
        }
        --output.num_joints;
        smallestBoneSize = std::min_element(numJointVertices.begin(), numJointVertices.end());
    }
    return static_cast<int>(numJointVertices.size());
}

int findMostStableVertex(const Input& input)
{
    const int num_vertices = input.num_vertices;
    const int num_samples = input.num_samples;
    double minErrorSq = std::numeric_limits<double>::max();
    int mostStableVertex = -1;
    for (int v = 0; v < num_vertices; ++v) {
        double errSq = 0;
        for (int s = 0; s < num_samples; ++s) {
            float3 diff = input.sample[s * num_vertices + v] - input.restShape[v];
            errSq += diff * diff;
        }
        if (errSq < minErrorSq) {
            minErrorSq = errSq;
            mostStableVertex = v;
        }
    }
    return mostStableVertex;
}

int findDistantVertex(const Input& input, const Output& output, const std::set<int>& covered)
{
    const int num_vertices = input.num_vertices;
    const int num_samples  = input.num_samples;
    const int num_indices  = output.num_indices;
    double maxErrorSq      = -std::numeric_limits<double>::max();
    int mostDistantVertex = -1;
    for (int v = 0; v < num_vertices; ++v) {
        if (covered.find(v) != covered.end()) {
            continue;
        }
        int index = output.skinIndex[v * num_indices + 0];
        double errSq = 0;
        for (int s = 0; s < num_samples; ++s) {
            float4x4 bm = output.skinMatrix[s * input.num_joints + index].asMatrix();
            float3 diff = input.sample[s * num_vertices + v] - input.restShape[v] * bm;
            errSq += diff * diff;
        }
        if (errSq > maxErrorSq) {
            maxErrorSq = errSq;
            mostDistantVertex = v;
        }
    }
    return mostDistantVertex;
}


// solving p-center problem
static void clusterVerticesPcenter(Output& output, const Input& input, int transformType)
{
    const int& num_indices = output.num_indices;

    output.num_joints = input.num_joints;
    std::fill(output.skinIndex.begin(), output.skinIndex.end(), -1);
    std::fill(output.skinWeight.begin(), output.skinWeight.end(), 0.0);
    std::fill(output.skinMatrix.begin(), output.skinMatrix.end(), float4x4::identity);
    std::fill(output.restJointPos.begin(), output.restJointPos.end(), float3::origin);

    char buf[512];
    int stable_vertex = findMostStableVertex(input);
    output.weights[stable_vertex * num_indices] = { 0, 1.0f };
    output.restJointPos[0] = input.restShape[stable_vertex];
    std::vector<int> joint_vertices(output.num_joints);
    joint_vertices[0] = stable_vertex;
    sprintf(buf, "Added to Vertex %d (stable)", stable_vertex);

    for (int jid = 1; jid < input.num_joints; ++jid) {
        double max_dist = -1.0;
        int distant_vertex = -1;
        for (int i = 0; i < input.num_vertices; ++i) {
            if (std::find(joint_vertices.begin(), joint_vertices.end(), i) != joint_vertices.end()) {
                continue;
            }
            double mindist = std::numeric_limits<double>::max();
            for (int j = 0; j < jid; ++j) {
                const int joint = joint_vertices[j];
                float3 v = input.restShape[i] - input.restShape[joint];
                if (mindist > v.length()) {
                    mindist = v.length();
                }
            }
            if (mindist > max_dist)
            {
                max_dist = mindist;
                distant_vertex = i;
            }
        }
        output.restJointPos[jid] = input.restShape[distant_vertex];
        joint_vertices[jid] = distant_vertex;
        sprintf(buf, "Added to Vertex %d, %f", distant_vertex, max_dist);
    }

    for (int i = 0; i < input.num_vertices; ++i) {
        double min_dist = std::numeric_limits<double>::max();
        int nearest_joint = -1;
        for (int j = 0; j < output.num_joints; ++j) {
            const int joint = joint_vertices[j];
            float3 v = input.restShape[i] - input.restShape[joint];
            if (v.length() < min_dist) {
                min_dist = v.length();
                nearest_joint = j;
            }
        }
        output.skinIndex[i * num_indices] = nearest_joint;
        output.skinWeight[i * num_indices] = 1.0;
    }
    detectNeighborClusters(input, output);
    updateJointTransformProc(output, transformType, input);
}

static void clusterVerticesAdaptive(Output& output, const Input& input, int transformType)
{
    const int& num_indices = output.num_indices;

    output.num_joints = 0;
    std::fill(output.skinIndex.begin(), output.skinIndex.end(), -1);
    std::fill(output.skinWeight.begin(), output.skinWeight.end(), 0.0);
    std::fill(output.skinMatrix.begin(), output.skinMatrix.end(), float4x4::identity);
    output.restJointPos.clear();

    std::set<int> covered;
    char buf[512];

    int stable_vertex = findMostStableVertex(input);
    output.skinIndex[stable_vertex * num_indices] = 0;
    output.skinWeight[stable_vertex * num_indices] = 1.0;
    output.restJointPos.push_back(input.restShape[stable_vertex]);
    covered.insert(stable_vertex);
    sprintf(buf, "Added to Vertex %d (stable)", stable_vertex);

    for (int i = 0; i < input.neighbor[stable_vertex].size(); ++i) {
        int neighbor = input.neighbor[stable_vertex][i].first;
        if (neighbor < 0) {
            continue;
        }
        output.skinIndex[neighbor * num_indices] = 0;
        output.skinWeight[neighbor * num_indices] = 1.0;
        covered.insert(neighbor);
    }
    updateJointTransformProc(output, transformType, input, 0);
    for (int v = 0; v < input.num_vertices; ++v) {
        output.skinIndex[v * num_indices] = 0;
        output.skinWeight[v * num_indices] = 1.0;
    }
    output.num_joints = 1;

    for (int iteration = 0; iteration < input.num_joints + 10; ++iteration) {
        if (output.num_joints >= input.num_joints || covered.size() >= input.num_vertices) {
            break;
        }
        int distant_vertex = findDistantVertex(input, output, covered);
        output.skinIndex[distant_vertex * num_indices] = output.num_joints;
        output.restJointPos.push_back(input.restShape[distant_vertex]);
        covered.insert(distant_vertex);
        for (int i = 0; i < input.neighbor[distant_vertex].size(); ++i) {
            int neighbor = input.neighbor[distant_vertex][i].first;
            if (neighbor < 0) {
                continue;
            }
            output.skinIndex[neighbor * num_indices] = output.num_joints;
            covered.insert(neighbor);
        }
        updateJointTransformProc(output, transformType, input, output.num_joints);
        ++output.num_joints;
        output.num_joints = bindVertexToJoint(output, input);

        sprintf(buf, "Added to Vertex %d", distant_vertex);
    }
    detectNeighborClusters(input, output);
}

static void initialize(Output& output, Input& input)
{
    PyArrayObject* initPos = reinterpret_cast<PyArrayObject*>(PyTuple_GET_ITEM(args, 0));
    PyArrayObject* shapeSample = reinterpret_cast<PyArrayObject*>(PyTuple_GET_ITEM(args, 1));
    PyArrayObject* neighborVertices = reinterpret_cast<PyArrayObject*>(PyTuple_GET_ITEM(args, 2));
    const long numJoints = PyInt_AsLong(PyTuple_GET_ITEM(args, 3));
    const long numIndices = PyInt_AsLong(PyTuple_GET_ITEM(args, 4));
    const long numRings = PyInt_AsLong(PyTuple_GET_ITEM(args, 5));
    const long numVertices = static_cast<long>(initPos->dimensions[0]);
    const long numSamples = static_cast<long>(shapeSample->dimensions[0]);
    // allocation
    input.numVertices = numVertices;
    input.numSamples = numSamples;
    input.numJoints = numJoints;
    input.numRings = numRings;
    input.rest_shape.points.resize(numVertices);
    input.sample.resize(numSamples);
    input.neighbor.resize(numVertices);

    output.joints.resize(numJoints);
    for (auto& jd : output.joints) {
        jd.weights.resize(numVertices);
        jd.matrix.resize(numSamples);
    }

    output.skinMatrix = std::vector<MTransformationMatrix>(numSamples * numJoints, MTransformationMatrix::identity);
    output.restJointPos = std::vector<MPoint>(numJoints);
    output.vertCluster = std::vector<std::vector<int>>(numVertices);

    // size normalization
    MVector centroid(0, 0, 0);
    MVector minPos(1.0e10, 1.0e10, 1.0e10), maxPos(-1.0e10, -1.0e10, -1.0e10);
    for (int v = 0; v < numVertices; ++v) {
        MVector p(GET_DOUBLE2(initPos, v, 0),
            GET_DOUBLE2(initPos, v, 1),
            GET_DOUBLE2(initPos, v, 2));
        input->restShape[v] = p;
        centroid += p;
        for (int i = 0; i < 3; ++i) {
            minPos[i] = std::min(minPos[i], p[i]);
            maxPos[i] = std::max(maxPos[i], p[i]);
        }
    }
    centroid /= numVertices;
    double scale = std::max(std::max(maxPos.x - minPos.x, maxPos.y - minPos.y), maxPos.z - minPos.z);
    MMatrix m = MMatrix::identity;
    m(0, 0) = m(1, 1) = m(2, 2) = 1.0 / scale;
    m(3, 0) = -centroid.x / scale;
    m(3, 1) = -centroid.y / scale;
    m(3, 2) = -centroid.z / scale;
    input->normalizer = m;
    // normalized samples
    for (int v = 0; v < numVertices; ++v) {
        input->restShape[v] = input->restShape[v] * input->normalizer;
        for (int s = 0; s < numSamples; ++s) {
            MPoint p(GET_DOUBLE3(shapeSample, s, v, 0),
                GET_DOUBLE3(shapeSample, s, v, 1),
                GET_DOUBLE3(shapeSample, s, v, 2));
            input->sample[s * numVertices + v] = p * input->normalizer;
        }
    }

    // one-ring neighbor [Le and Deng 2014]
    for (int v = 0; v < numVertices; ++v) {
        double lv = 0.0;
        for (int i = 0; i < neighborVertices->dimensions[1]; ++i) {
            long n = GET_LONG2(neighborVertices, v, i);
            if (n < 0) {
                continue;
            }
            double len = (input->restShape[v] - input->restShape[n]).length();
            if (len > 0) {
                // [Le and Deng 2014]
                double dsqsum = 0;
                for (int s = 0; s < input->numSamples; ++s) {
                    float diff = (input->restShape[v] - input->restShape[n]).length()
                        - (input->sample[s * numVertices + v] - input->sample[s * numVertices + n]).length();
                    dsqsum += diff * diff;
                }
                dsqsum += 1.0e-10;
                double dvn = 1.0 / std::sqrt(dsqsum / input->numSamples);
                input->neighbor[v].push_back(std::make_pair(n, dvn));
                lv += dvn;
            }
        }
        for (int n = 0; n < input->neighbor[v].size(); ++n) {
            input->neighbor[v][n].second /= -lv;
        }
    }
}
#endif

} // namespace ssds

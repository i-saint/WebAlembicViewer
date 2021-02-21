#include "pch.h"
#include "WebAlembicViewer.h"

namespace wabc {

class Renderer : public IRenderer
{
public:
    Renderer();
    ~Renderer();
    void release() override;
    bool initialize(GLFWwindow* v) override;

    void beginScene() override;
    void endScene() override;
    void setCamera(float3 pos, float3 target, float fov, float near_, float far_) override;
    void draw(IMesh* mesh) override;
    void draw(IPoints* points) override;

private:
    GLFWwindow* m_window{};

    GLuint m_vs_fill{};
    GLuint m_fs_fill{};
    GLuint m_shader_fill{};

    GLuint m_loc_view_proj{};
    GLuint m_loc_point_size{};
    GLuint m_loc_color{};
    GLuint m_attr_point{};

    float4x4 m_view_proj = float4x4::identity();
    float4 m_color = {0.0f, 0.0f, 0.0f, 1.0f};
    float m_point_size = 4.0f;
};




static const char* g_vs_fill_src = R"(
uniform mat4 g_view_proj;
uniform float g_point_size;
attribute vec3 g_point;

void main()
{
    gl_Position = g_view_proj * vec4(g_point, 1.0);
    gl_PointSize = g_point_size;
}
)";

static const char* g_fs_fill_src = R"(
precision mediump float;
uniform vec4 g_color;

void main()
{
    gl_FragColor = g_color;
}
)";


static void CheckError(GLuint shader)
{
    GLint result;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        std::string log;
        GLint len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        log.resize(len);

        GLsizei length;
        glGetShaderInfoLog(shader, log.size(), &length, log.data());

        puts(log.c_str());
    }
}

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
    glDeleteShader(m_vs_fill);
    glDeleteShader(m_fs_fill);
    glDeleteProgram(m_shader_fill);
}

bool Renderer::initialize(GLFWwindow* v)
{
    m_window = v;

    m_vs_fill = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(m_vs_fill, 1, &g_vs_fill_src, nullptr);
    glCompileShader(m_vs_fill);
    CheckError(m_vs_fill);

    m_fs_fill = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(m_fs_fill, 1, &g_fs_fill_src, nullptr);
    glCompileShader(m_fs_fill);
    CheckError(m_vs_fill);

    m_shader_fill = glCreateProgram();
    glAttachShader(m_shader_fill, m_vs_fill);
    glAttachShader(m_shader_fill, m_fs_fill);
    glLinkProgram(m_shader_fill);

    m_loc_view_proj = glGetUniformLocation(m_shader_fill, "g_view_proj");
    m_loc_point_size = glGetUniformLocation(m_shader_fill, "g_point_size");
    m_loc_color = glGetUniformLocation(m_shader_fill, "g_color");
    m_attr_point = glGetAttribLocation(m_shader_fill, "g_point");

    glEnableVertexAttribArray(m_attr_point);
    glVertexAttribPointer(m_attr_point, 3, GL_FLOAT, GL_FALSE, sizeof(float3), nullptr);

    m_view_proj = transpose(orthographic(-100.0f, 100.0f, -100.0f, 100.0f, 0.0f, 100.0f));

    return true;
}

void Renderer::release()
{
    delete this;
}

void Renderer::beginScene()
{
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);
    float aspect = (float)w / (float)h;

    glViewport(0, 0, w, h);
    glClearColor(0.25f, 0.25f, 0.25f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_shader_fill);
    glUniformMatrix4fv(m_loc_view_proj, 1, GL_FALSE, (const GLfloat*)&m_view_proj);
    glUniform1fv(m_loc_point_size, 1, (const GLfloat*)&m_point_size);
    glUniform4fv(m_loc_color, 1, (const GLfloat*)&m_color);
}

void Renderer::endScene()
{
    glFlush();
    glfwSwapBuffers(m_window);
}

void Renderer::setCamera(float3 pos, float3 target, float fov, float near_, float far_)
{
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);
    float aspect = (float)w / (float)h;

    //quatf rot = look_quat(dir, float3::up());
    //float4x4 view = invert(transform(pos, rot));
    //float4x4 proj = perspective(fov, aspect, near_, far_);

    float4x4 proj = float4x4::identity();
    float4x4 view = float4x4::identity();
    {
        float3 f = normalize(target - pos);
        float3 s = normalize(cross(f, float3::up()));
        float3 u = cross(s, f);

        (float3&)view[0] = s;
        (float3&)view[1] = u;
        (float3&)view[2] = f;
        view[0][3] = -dot(s, pos);
        view[1][3] = -dot(u, pos);
        view[2][3] = dot(f, pos);
        view = transpose(view);
    }
    {
        float f = std::tan(fov * DegToRad / 2.0f);
        proj[0][0] = 1.0f / (aspect * f);
        proj[1][1] = 1.0f / (f);
        proj[2][2] = -(far_ + near_) / (far_ - near_);
        proj[2][3] = -1.0f;
        proj[3][2] = -(2.0f * far_ * near_) / (far_ - near_);
    }

    m_view_proj = (view * proj);
}

void Renderer::draw(IMesh* v)
{
    if (!v)
        return;

    auto points = v->getPoints();
    if (points.empty())
        return;
    auto points_ex = v->getPointsEx();
    auto wireframe_indices = v->getWireframeIndices();

    //// triangles
    //{
    //    glBindBuffer(GL_ARRAY_BUFFER, v->getPointsExBuffer());
    //    glEnableVertexAttribArray(m_attr_point);
    //    glVertexAttribPointer(m_attr_point, 3, GL_FLOAT, GL_FALSE, sizeof(float3), nullptr);

    //    glDrawArrays(GL_TRIANGLES, 0, points_ex.size());

    //    glBindBuffer(GL_ARRAY_BUFFER, 0);
    //}

    // wire frame
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, v->getWireframeIndicesBuffer());
        glBindBuffer(GL_ARRAY_BUFFER, v->getPointsBuffer());
        glEnableVertexAttribArray(m_attr_point);
        glVertexAttribPointer(m_attr_point, 3, GL_FLOAT, GL_FALSE, sizeof(float3), nullptr);

        glDrawElements(GL_LINES, wireframe_indices.size(), GL_UNSIGNED_INT, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // points
    {
        glBindBuffer(GL_ARRAY_BUFFER, v->getPointsBuffer());
        glEnableVertexAttribArray(m_attr_point);
        glVertexAttribPointer(m_attr_point, 3, GL_FLOAT, GL_FALSE, sizeof(float3), nullptr);

        glDrawArrays(GL_POINTS, 0, points.size());

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void Renderer::draw(IPoints* v)
{
    if (!v)
        return;

    auto points = v->getPoints();
    if (points.empty())
        return;

    glBindBuffer(GL_ARRAY_BUFFER, v->getPointBuffer());
    glEnableVertexAttribArray(m_attr_point);
    glVertexAttribPointer(m_attr_point, 3, GL_FLOAT, GL_FALSE, sizeof(float3), nullptr);
    glDrawArrays(GL_POINTS, 0, points.size());
}

IRenderer* CreateRenderer_()
{
    return new Renderer();
}

} // namespace wabc

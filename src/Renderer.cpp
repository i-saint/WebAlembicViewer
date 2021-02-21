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

private:
    GLFWwindow* m_window{};

    GLuint m_vs_fill{};
    GLuint m_fs_fill{};
    GLuint m_shader_fill{};

    GLuint m_loc_view_proj{};
    GLuint m_loc_point_size{};
    GLuint m_loc_color{};
    GLuint m_attr_point{};

    GLuint m_vb{};

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

    glDeleteBuffers(1, &m_vb);
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

    glGenBuffers(1, &m_vb);

    //std::vector<float3> points{
    //    float3{-0.6f, -0.4f, 0.0f} *1.0f,
    //    float3{ 0.6f, -0.4f, 0.0f} *1.0f,
    //    float3{ 0.0f,  0.6f, 0.0f} *1.0f,
    //};
    //glBindBuffer(GL_ARRAY_BUFFER, m_vb);
    //glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(float3), points.data(), GL_DYNAMIC_DRAW);

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
        float3 f(normalize(target - pos));
        float3 s(normalize(cross(f, float3::up())));
        float3 u(cross(s, f));

        view[0][0] = s.x;
        view[1][0] = s.y;
        view[2][0] = s.z;
        view[0][1] = u.x;
        view[1][1] = u.y;
        view[2][1] = u.z;
        view[0][2] = -f.x;
        view[1][2] = -f.y;
        view[2][2] = -f.z;
        view[3][0] = -dot(s, pos);
        view[3][1] = -dot(u, pos);
        view[3][2] = dot(f, pos);
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

void Renderer::draw(IMesh* mesh)
{
    if (!mesh)
        return;

    auto points = mesh->getPoints();
    glBindBuffer(GL_ARRAY_BUFFER, m_vb);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(float3), points.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(m_attr_point);
    glVertexAttribPointer(m_attr_point, 3, GL_FLOAT, GL_FALSE, sizeof(float3), nullptr);

    glDrawArrays(GL_POINTS, 0, points.size());
    glDrawArrays(GL_LINES, 0, points.size());
}

IRenderer* CreateRenderer_()
{
    return new Renderer();
}

} // namespace wabc

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
    void setCamera(ICamera* cam) override;
    void draw(IMesh* mesh) override;

private:
    GLFWwindow* m_window{};

    GLuint m_vs{};
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




static const char* g_vertex_shader_src = R"(
uniform mat4 g_view_proj;
uniform float g_point_size;
attribute vec3 g_point;

void main()
{
    gl_Position = g_view_proj * vec4(g_point, 1.0);
    gl_PointSize = g_point_size;
}
)";

static const char* g_fragment_shader_src = R"(
precision mediump float;
uniform vec4 g_color;

void main()
{
    gl_FragColor = g_color;
}
)";

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
    glDeleteShader(m_vs);
    glDeleteShader(m_fs_fill);
    glDeleteProgram(m_shader_fill);

    glDeleteBuffers(1, &m_vb);
}

bool Renderer::initialize(GLFWwindow* v)
{
    m_window = v;

    m_vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(m_vs, 1, &g_vertex_shader_src, nullptr);
    glCompileShader(m_vs);

    m_fs_fill = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(m_fs_fill, 1, &g_fragment_shader_src, nullptr);
    glCompileShader(m_fs_fill);

    m_shader_fill = glCreateProgram();
    glAttachShader(m_shader_fill, m_vs);
    glAttachShader(m_shader_fill, m_fs_fill);
    glLinkProgram(m_shader_fill);

    m_loc_view_proj = glGetUniformLocation(m_shader_fill, "g_view_proj");
    m_loc_point_size = glGetUniformLocation(m_shader_fill, "g_point_size");
    m_loc_color = glGetUniformLocation(m_shader_fill, "g_color");
    m_attr_point = glGetAttribLocation(m_shader_fill, "g_point");

    glEnableVertexAttribArray(m_attr_point);
    glVertexAttribPointer(m_attr_point, 3, GL_FLOAT, GL_FALSE, sizeof(float3), nullptr);

    glGenBuffers(1, &m_vb);

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
    glfwSwapBuffers(m_window);
}

void Renderer::setCamera(ICamera* cam)
{
}

void Renderer::draw(IMesh* mesh)
{
    glBindVertexArray(m_vb);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

IRenderer* CreateRenderer_()
{
    return new Renderer();
}

} // namespace wabc

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
    void setCamera(float3 pos, float3 dir, float3 up, float fov, float near_, float far_) override;
    void setCamera(ICamera* cam) override;
    void setDrawPoints(bool v) override { m_draw_points = v; }
    void setDrawWireframe(bool v) override { m_draw_wireframe = v; }
    void setDrawFaces(bool v) override { m_draw_faces = v; }

    void beginDraw() override;
    void endDraw() override;
    void draw(IMesh* mesh) override;
    void draw(IPoints* points) override;

private:
    GLFWwindow* m_window{};

    GLuint m_vs_fill{};
    GLuint m_fs_fill{};
    GLuint m_shader_fill{};

    GLuint m_u_mvp{};
    GLuint m_u_point_size{};
    GLuint m_u_color{};
    GLuint m_ia_point{};
    GLuint m_ia_normal{};

    float4x4 m_view_proj = float4x4::identity();
    float4 m_face_color = { 0.4f, 0.4f, 0.4f, 1.0f };
    float4 m_fill_color = { 0.0f, 0.0f, 0.0f, 1.0f };
    float m_point_size = 4.0f;

    bool m_draw_points = false;
    bool m_draw_wireframe = true;
    bool m_draw_faces = true;
};




static const char* g_vs_fill_src = R"(
uniform mat4 u_mvp;
uniform float u_point_size;
attribute vec3 ia_point;
attribute vec3 ia_normal;
varying vec3 vs_normal;

void main()
{
    gl_Position = u_mvp * vec4(ia_point, 1.0);
    vs_normal = ia_normal;
    gl_PointSize = u_point_size;
}
)";

static const char* g_fs_fill_src = R"(
precision mediump float;
uniform vec4 u_color;
varying vec3 vs_normal;

void main()
{
    gl_FragColor = u_color;
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

    m_u_mvp         = glGetUniformLocation(m_shader_fill, "u_mvp");
    m_u_point_size  = glGetUniformLocation(m_shader_fill, "u_point_size");
    m_u_color       = glGetUniformLocation(m_shader_fill, "u_color");
    m_ia_point      = glGetAttribLocation(m_shader_fill, "ia_point");
    m_ia_normal     = glGetAttribLocation(m_shader_fill, "ia_normal");

    glEnableVertexAttribArray(m_ia_point);
    glVertexAttribPointer(m_ia_point, 3, GL_FLOAT, GL_FALSE, sizeof(float3), nullptr);

    m_view_proj = transpose(orthographic(-100.0f, 100.0f, -100.0f, 100.0f, 0.0f, 100.0f));

    return true;
}

void Renderer::release()
{
    delete this;
}

void Renderer::beginDraw()
{
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);

    glViewport(0, 0, w, h);
    glClearColor(0.25f, 0.25f, 0.25f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_shader_fill);
    glUniformMatrix4fv(m_u_mvp, 1, GL_FALSE, (const GLfloat*)&m_view_proj);
    glUniform1fv(m_u_point_size, 1, (const GLfloat*)&m_point_size);
    glUniform4fv(m_u_color, 1, (const GLfloat*)&m_fill_color);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
}

void Renderer::endDraw()
{
    glDisable(GL_DEPTH_TEST);

    glFlush();
    glfwSwapBuffers(m_window);
}

void Renderer::setCamera(float3 pos, float3 dir, float3 up, float fov, float near_, float far_)
{
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);
    float aspect = (float)w / (float)h;

    float4x4 proj = float4x4::identity();
    float4x4 view = float4x4::identity();
    {
        float3 z = -dir;
        float3 x = normalize(cross(up, z));
        float3 y = cross(z, x);
        view[0] = { x.x, y.x, z.x, 0.0f };
        view[1] = { x.y, y.y, z.y, 0.0f };
        view[2] = { x.z, y.z, z.z, 0.0f };
        view[3] = { -dot(x, pos), -dot(y, pos), -dot(z, pos), 1.0f };
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

void Renderer::setCamera(ICamera* cam)
{
    if (!cam)
        return;
    setCamera(cam->getPosition(), cam->getDirection(), cam->getUp(), cam->getFOV(), cam->getNearPlane(), cam->getFarPlane());
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

    // faces
    if (m_draw_faces) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);

        glUseProgram(m_shader_fill);
        glUniform4fv(m_u_color, 1, (const GLfloat*)&m_face_color);

        glBindBuffer(GL_ARRAY_BUFFER, v->getPointsExBuffer());
        glEnableVertexAttribArray(m_ia_point);
        glVertexAttribPointer(m_ia_point, 3, GL_FLOAT, GL_FALSE, sizeof(float3), nullptr);

        glDrawArrays(GL_TRIANGLES, 0, points_ex.size());

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    // wire frame
    if (m_draw_wireframe) {
        glDepthMask(GL_FALSE);

        glUseProgram(m_shader_fill);
        glUniform4fv(m_u_color, 1, (const GLfloat*)&m_fill_color);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, v->getWireframeIndicesBuffer());
        glBindBuffer(GL_ARRAY_BUFFER, v->getPointsBuffer());
        glEnableVertexAttribArray(m_ia_point);
        glVertexAttribPointer(m_ia_point, 3, GL_FLOAT, GL_FALSE, sizeof(float3), nullptr);

        glDrawElements(GL_LINES, wireframe_indices.size(), GL_UNSIGNED_INT, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDepthMask(GL_TRUE);
    }

    // points
    if (m_draw_points) {
        glDepthMask(GL_FALSE);

        glUseProgram(m_shader_fill);
        glUniform4fv(m_u_color, 1, (const GLfloat*)&m_fill_color);

        glBindBuffer(GL_ARRAY_BUFFER, v->getPointsBuffer());
        glEnableVertexAttribArray(m_ia_point);
        glVertexAttribPointer(m_ia_point, 3, GL_FLOAT, GL_FALSE, sizeof(float3), nullptr);

        glDrawArrays(GL_POINTS, 0, points.size());

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDepthMask(GL_TRUE);
    }
}

void Renderer::draw(IPoints* v)
{
    if (!v)
        return;

    auto points = v->getPoints();
    if (points.empty())
        return;

    {
        glDepthMask(GL_FALSE);

        glUseProgram(m_shader_fill);
        glUniform4fv(m_u_color, 1, (const GLfloat*)&m_fill_color);

        glBindBuffer(GL_ARRAY_BUFFER, v->getPointBuffer());
        glEnableVertexAttribArray(m_ia_point);
        glVertexAttribPointer(m_ia_point, 3, GL_FLOAT, GL_FALSE, sizeof(float3), nullptr);

        glDrawArrays(GL_POINTS, 0, points.size());

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDepthMask(GL_TRUE);
    }
}

IRenderer* CreateRenderer_()
{
    return new Renderer();
}

} // namespace wabc

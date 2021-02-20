#include "pch.h"
#include "WebAlembicViewer.h"

#pragma comment(lib, "Alembic.lib")
#pragma comment(lib, "Half-2_5.lib")
#pragma comment(lib, "Iex-2_5.lib")
#pragma comment(lib, "IexMath-2_5.lib")
#pragma comment(lib, "Imath-2_5.lib")
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "libEGL.dll.lib")
#pragma comment(lib, "libGLESv2.dll.lib")


static wabc::IScenePtr g_scene;
static wabc::IRendererPtr g_renderer;
static GLFWwindow* g_window;


wabcAPI bool wabcLoadScene(const char* path)
{
    g_scene = wabc::CreateScene();
    if (g_scene->load(path)) {
        return true;
    }
    else {
        g_scene = nullptr;
        return false;
    }
}

wabcAPI double wabcGetStartTime()
{
    return g_scene ? std::get<0>(g_scene->getTimeRange()) : 0.0;
}

wabcAPI double wabcGetEndTime()
{
    return g_scene ? std::get<1>(g_scene->getTimeRange()) : 0.0;
}

wabcAPI void wabcSeek(double t)
{
    if (g_scene) {
        g_scene->seek(t);
    }
}

#ifdef __EMSCRIPTEN__
void Draw()
#else
void Draw(GLFWwindow*)
#endif
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glUseProgram(shader_programme);
    //glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfwSwapBuffers(g_window);
}

int main(int argc, char** argv)
{
    //if (argc < 2) {
    //    printf("no input files\n");
    //    return 0;
    //}
    if (!glfwInit()) {
        printf("ERROR: could not start GLFW3\n");
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    g_window = glfwCreateWindow(300, 300, "Web Alembic Viewer", nullptr, nullptr);
    glfwMakeContextCurrent(g_window);

    g_scene = wabc::CreateScene();
    g_renderer = wabc::CreateRenderer();

    if (argc >= 2) {
        if (g_scene->load(argv[1])) {
            auto time_range = g_scene->getTimeRange();
            printf("%s\n", argv[1]);
            printf("time range: %lf %lf\n", std::get<0>(time_range), std::get<1>(time_range));

            g_scene->seek(0.0);
            auto mesh = g_scene->getMesh();
            printf("vertex count: %d\n", (int)mesh->getPoints().size());
        }
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(&Draw, 0, 1);
#else
    glfwSetWindowRefreshCallback(g_window , &Draw);
    while (!glfwWindowShouldClose(g_window)) {
        glfwWaitEvents();
        //glfwPollEvents();
    }
#endif
    glfwTerminate();
}

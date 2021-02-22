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

using wabc::float2;
using wabc::float3;
using wabc::float4;
using wabc::float3x3;
using wabc::float4x4;

static wabc::IScenePtr g_scene;
static wabc::IRendererPtr g_renderer;
static GLFWwindow* g_window;

static int g_key_mod;
static int g_mouse_button;
static float2 g_mouse_pos;
static float3 g_camera_position{ 0.0f, 0.0f, 10.0f };
static float3 g_camera_target{ 0.0f, 0.0f, 0.0f };
static int g_active_camera = -1; // -1: free
static wabc::IRenderer::FovType g_fov_type = wabc::IRenderer::FovType::Horizontal;
static float g_camera_fov = 60.0f;
static float g_camera_near = 0.01f;
static float g_camera_far = 100.0f;
static double g_seek_time;

static void Draw()
{
    if (g_active_camera < 0) {
        float3 dir = normalize(g_camera_target - g_camera_position);
        float3 up = float3::up();
        g_renderer->setCamera(g_camera_position, dir, up, g_camera_fov, g_camera_near, g_camera_far);
    }
    else {
        g_renderer->setCamera(g_scene->getCameras()[g_active_camera], g_fov_type);
    }

    g_renderer->beginDraw();
    g_renderer->draw(g_scene->getMesh());
    g_renderer->draw(g_scene->getPoints());
    g_renderer->endDraw();
}

static void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;

        case GLFW_KEY_RIGHT_CONTROL:
        case GLFW_KEY_LEFT_CONTROL:
            g_key_mod |= GLFW_MOD_CONTROL;
            break;

        default:
            break;
        }
    }
    else if (action == GLFW_RELEASE) {
        switch (key) {
        case GLFW_KEY_RIGHT_CONTROL:
        case GLFW_KEY_LEFT_CONTROL:
            g_key_mod &= ~GLFW_MOD_CONTROL;
            break;

        default:
            break;
        }
    }
}

static void OnMouseButton(GLFWwindow* window, int button, int action, int mod)
{
    //printf("OnMouseButton() %d %d %d\n", button, action, mod);

    if (action == GLFW_PRESS)
        g_mouse_button |= (1 << button);
    else if (action == GLFW_RELEASE)
        g_mouse_button &= ~(1 << button);
}

static void OnMouseMove(GLFWwindow* window, double x, double y)
{
    //printf("OnMouseMove() %lf %lf\n", x, y);

    using namespace wabc;
    float2 pos = float2{ (float)x, (float)y };
    float2 move = pos - g_mouse_pos;
    g_mouse_pos = pos;

    if (g_key_mod == 0) {
        if ((g_mouse_button & 1) != 0) {
            // rotate
            float3 axis = cross(normalize(g_camera_target - g_camera_position), float3::up());
            g_camera_position = to_mat3x3(rotate_y(move.x * DegToRad * 0.1f)) * g_camera_position;
            g_camera_position = to_mat3x3(rotate(axis, move.y * DegToRad * 0.1f)) * g_camera_position;
        }
        else if ((g_mouse_button & 2) != 0) {
            // move
            float len = length(g_camera_target - g_camera_position);
            float3 t = float3{ move.x, move.y, 0.0f } *0.001f * len;

            float3 dir = normalize(g_camera_target - g_camera_position);
            quatf rot = look_quat(dir, float3::up());
            t = to_mat3x3(rot) * t;

            g_camera_position += t;
            g_camera_target += t;
        }
    }
    else if (g_key_mod & GLFW_MOD_CONTROL) {
        if (g_scene) {
            double t = move.x * 0.005;
            auto range = g_scene->getTimeRange();
            g_seek_time = clamp(g_seek_time + t, std::get<0>(range), std::get<1>(range));
            g_scene->seek(g_seek_time);
        }
    }
}

static void OnScroll(GLFWwindow* window, double x, double y)
{
    //printf("OnScroll() %lf %lf\n", x, y);
#ifdef __EMSCRIPTEN__
    x /= 125.0;
    y /= -125.0;
#endif

    using namespace wabc;
    if (y != 0.0) {
        // zoom
        float s = -y * 0.1f + 1.0f;
        float d = length(g_camera_target - g_camera_position) * s;
        float3 dir = normalize(g_camera_position - g_camera_target);
        g_camera_position = g_camera_target + (dir * d);
    }
}


wabcAPI bool wabcLoadScene(std::string path)
{
    g_scene = wabc::CreateScene();
    if (g_scene->load(path.c_str())) {
        printf("wabcLoadScene(\"%s\"): succeeded\n", path.c_str());
        return true;
    }
    else {
        printf("wabcLoadScene(\"%s\"): failed\n", path.c_str());
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
        g_seek_time = t;
        g_scene->seek(g_seek_time);
    }
}

wabcAPI int wabcGetCameraCount()
{
    return g_scene ? (int)g_scene->getCameras().size() : 0;
}

std::string wabcGetCameraPath(int v)
{
    if (g_scene)
        return g_scene->getCameras()[v]->getPath();
    return "";
}

wabcAPI void wabcSetActiveCamera(int v)
{
    g_active_camera = v;
}

wabcAPI void wabcSetCameraFovType(int v)
{
    g_fov_type = (wabc::IRenderer::FovType)v;
}

wabcAPI void wabcSetFOV(float v)
{
    g_camera_fov = v;
}

wabcAPI void wabcSetDrawFaces(bool v)
{
    if (g_renderer)
        g_renderer->setDrawFaces(v);
}

wabcAPI void wabcSetDrawWireframe(float v)
{
    if (g_renderer)
        g_renderer->setDrawWireframe(v);
}

wabcAPI void wabcSetDrawPoints(float v)
{
    if (g_renderer)
        g_renderer->setDrawPoints(v);
}

wabcAPI void wabcDraw()
{
    Draw();
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(wabc) {
    using namespace emscripten;
    function("wabcLoadScene", &wabcLoadScene);
    function("wabcGetStartTime", &wabcGetStartTime);
    function("wabcGetEndTime", &wabcGetEndTime);
    function("wabcSeek", &wabcSeek);

    function("wabcGetCameraCount", &wabcGetCameraCount);
    function("wabcGetCameraPath", &wabcGetCameraPath, allow_raw_pointers());
    function("wabcSetActiveCamera", &wabcSetActiveCamera);
    function("wabcSetCameraFovType", &wabcSetCameraFovType);
    function("wabcSetFOV", &wabcSetFOV);

    function("wabcSetDrawFaces", &wabcSetDrawFaces);
    function("wabcSetDrawWireframe", &wabcSetDrawWireframe);
    function("wabcSetDrawPoints", &wabcSetDrawPoints);
    function("wabcDraw", &wabcDraw);
}
#endif


int main(int argc, char** argv)
{
    if (!glfwInit()) {
        printf("glfwInit() failed\n");
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    g_window = glfwCreateWindow(1280, 720, "Web Alembic Viewer", nullptr, nullptr);
    if (!g_window) {
        printf("glfwCreateWindow() failed\n");
        return 1;
    }
    glfwSetKeyCallback(g_window, OnKey);
    glfwSetMouseButtonCallback(g_window, OnMouseButton);
    glfwSetCursorPosCallback(g_window, OnMouseMove);
    glfwSetScrollCallback(g_window, OnScroll);

    glfwMakeContextCurrent(g_window);

    g_scene = wabc::CreateScene();
    g_renderer = wabc::CreateRenderer();
    g_renderer->initialize(g_window);

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
    glfwSwapInterval(1);
    while (!glfwWindowShouldClose(g_window)) {
        Draw();
        glfwPollEvents();
    }
    glfwDestroyWindow(g_window);
    glfwTerminate();
#endif
}

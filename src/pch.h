#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <functional>
#include <memory>
#include <fstream>
#include <span>
#include <chrono>

#include <Alembic/AbcCoreAbstract/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
//#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcMaterial/All.h>


#define GLFW_INCLUDE_ES3
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
    #include <emscripten/bind.h>
    #define wabcAPI EMSCRIPTEN_KEEPALIVE extern "C" __attribute__((visibility("default")))
#else
    #include <EGL/egl.h>
    #ifdef _WIN32
        #define wabcAPI extern "C" __declspec(dllexport)
    #else
        #define wabcAPI extern "C" __attribute__((visibility("default")))
    #endif
#endif // __EMSCRIPTEN__

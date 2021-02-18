CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(WebAlembicViewer)

set(CMAKE_FIND_ROOT_PATH /usr/local/emscripten)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
find_package(OpenEXR REQUIRED)
find_package(Alembic REQUIRED)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE PATH "" FORCE)
endif()
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/_dist" CACHE PATH "" FORCE)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fvisibility=hidden")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fvisibility=hidden -std=c++17 -Wno-deprecated -fpermissive")

file(GLOB sources *.cpp *.h)
add_executable(WebAlembicViewer ${sources})
target_include_directories(WebAlembicViewer
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${ALEMBIC_INCLUDE_DIRS}
)
target_link_libraries(WebAlembicViewer
    PRIVATE
        ${ALEMBIC_LIBRARIES}
)
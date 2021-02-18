find_path(OPENEXR_INCLUDE_DIR OpenEXR/half.h
    PATHS
        /usr/local/emscripten/include
    NO_DEFAULT_PATH
)
mark_as_advanced(OPENEXR_INCLUDE_DIR)

foreach(LIB Half Iex IexMath Imath IlmImf IlmThread)
    find_library(OPENEXR_${LIB}_LIBRARY "${LIB}-2_5"
        PATHS
            /usr/local/emscripten/lib
        NO_DEFAULT_PATH
    )
    mark_as_advanced(OPENEXR_${LIB}_LIBRARY)

    if(OPENEXR_${LIB}_LIBRARY)
        list(APPEND OPENEXR_LIBRARIES ${OPENEXR_${LIB}_LIBRARY})
    endif()
endforeach(LIB)

list(APPEND OPENEXR_INCLUDE_DIRS ${OPENEXR_INCLUDE_DIR})
list(APPEND OPENEXR_INCLUDE_DIRS ${OPENEXR_INCLUDE_DIR}/OpenEXR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenEXR
    REQUIRED_VARS
        OPENEXR_INCLUDE_DIRS
        OPENEXR_LIBRARIES
)

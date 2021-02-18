# * ALEMBIC_INCLUDE_DIR
# * ALEMBIC_LIBRARY

find_path(ALEMBIC_INCLUDE_DIR Alembic/Abc/All.h
    PATHS
        /usr/local/emscripten/include
    NO_DEFAULT_PATH
)
mark_as_advanced(ALEMBIC_INCLUDE_DIR)


foreach(LIB Alembic hdf5 szip z)
    find_library(ALEMBIC_${LIB}_LIBRARY ${LIB}
        PATHS
            /usr/local/emscripten/lib
        NO_DEFAULT_PATH
    )
    mark_as_advanced(ALEMBIC_${LIB}_LIBRARY)

    if(ALEMBIC_${LIB}_LIBRARY)
        list(APPEND ALEMBIC_LIBRARIES ${ALEMBIC_${LIB}_LIBRARY})
    endif()
endforeach(LIB)

list(APPEND ALEMBIC_INCLUDE_DIRS ${ALEMBIC_INCLUDE_DIR})
list(APPEND ALEMBIC_INCLUDE_DIRS ${OPENEXR_INCLUDE_DIRS})

list(APPEND ALEMBIC_LIBRARIES ${OPENEXR_Half_LIBRARY})
list(APPEND ALEMBIC_LIBRARIES ${OPENEXR_Iex_LIBRARY})
list(APPEND ALEMBIC_LIBRARIES ${OPENEXR_IexMath_LIBRARY})
list(APPEND ALEMBIC_LIBRARIES ${OPENEXR_Imath_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args("ALEMBIC"
    DEFAULT_MSG
    ALEMBIC_INCLUDE_DIR
	ALEMBIC_LIBRARIES
)

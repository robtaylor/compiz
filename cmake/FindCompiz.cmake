# Find Compiz and provide all necessary variables and macros to compile software for it.

if (NOT COMPIZ_INTERNAL)

    include (FindPkgConfig)

    if (COMPIZ_FIND_REQUIRED)
	set (_req REQUIRED)
    endif ()

    pkg_check_modules (COMPIZ ${_req} compiz)

    if (COMPIZ_FOUND)
	set (CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH} ${COMPIZ_PREFIX}/share/compiz/cmake)
	include (CompizDefaults)
    endif ()
endif ()

